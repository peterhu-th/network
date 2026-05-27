#include "AuthService.h"
#include "../utils/MemoryCache.h"
#include "../utils/CryptoUtils.h"
#include "../utils/JwtUtils.h"
#include "../utils/IdGenerator.h"
#include "../core/logger.h"
#include <QDateTime>
#include <QRegularExpression>
#include <QtConcurrent>

namespace radar::network {

    AuthService::AuthService(std::shared_ptr<UserMapper> userMapper, std::shared_ptr<SmtpClient> smtpClient, QString jwtSecret)
        : m_userMapper(std::move(userMapper)), m_smtpClient(std::move(smtpClient)), m_jwtSecret(std::move(jwtSecret)) {}

    Result<UserEntity> buildUserEntity(const QString& email, const QString& password, int role) {
        UserEntity user;
        auto idRes = IdGenerator::instance().nextId();
        if (!idRes.isOk()) {
            return Result<UserEntity>::error("Failed to generate ID", ErrorCode::InvalidState);
        }
        user.id = QString::number(idRes.value());
        user.email = email;
        user.passwordHash = QString::fromStdString(CryptoUtils::hashPassword(password.toStdString()));
        user.role = role;
        user.status = 1;
        user.createdAt = QDateTime::currentDateTimeUtc();
        user.updatedAt = QDateTime::currentDateTimeUtc();
        return Result<UserEntity>::ok(user);
    }

    Result<void> AuthService::sendVerificationCode(const QString& email, const QString& action) const {
        auto existing = m_userMapper->findByEmail(email);
        bool isRegistered = existing.isOk() && existing.value().has_value();

        if (action == "register" && isRegistered) {
            return Result<void>::error("Email is already registered", ErrorCode::InvalidParam);
        }
        if (action == "reset" && !isRegistered) {
            return Result<void>::error("Email is not registered", ErrorCode::InvalidParam);
        }

        // 生成 6 位随机数
        int codeInt = QRandomGenerator::global()->bounded(100000, 1000000);
        std::string codeStr = std::to_string(codeInt);

        // 存入缓存 5分钟 (300秒)
        utils::MemoryCache::getInstance().set("VERIFY_" + email.toStdString(), codeStr, 300);

        std::string subject = action == "reset" ? "Audio Radar Password Reset Code" : "Audio Radar Registration Code";
        std::string body = action == "reset" ? 
                           "Your password reset verification code is: " + codeStr + "\r\nThis code will expire in 5 minutes." :
                           "Your registration verification code is: " + codeStr + "\r\nThis code will expire in 5 minutes.";

        // Asynchronously send email
        (void)QtConcurrent::run([smtp = m_smtpClient, email, subject, body]() {
            auto res = smtp->sendEmail(email.toStdString(), subject, body);
            if (!res.isOk()) {
                LOG_ERROR("Network", "Failed to send verification email to " + email + ": " + res.errorMessage());
            }
        });

        return Result<void>::ok();
    }

    Result<void> AuthService::registerUser(const QString& email, const QString& code, const QString& password) const {
        std::string cachedCode;
        if (!utils::MemoryCache::getInstance().get("VERIFY_" + email.toStdString(), cachedCode)) {
            return Result<void>::error("Verification code expired or not found", ErrorCode::InvalidParam);
        }

        if (QString::fromStdString(cachedCode) != code) {
            return Result<void>::error("Incorrect verification code", ErrorCode::InvalidParam);
        }

        // 防重放，验证成功后立即删除
        utils::MemoryCache::getInstance().remove("VERIFY_" + email.toStdString());

        // 检查邮箱是否已注册
        QRegularExpression emailRegex("^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$");
        if (!emailRegex.match(email).hasMatch()) {
            return Result<void>::error("Invalid email format", ErrorCode::InvalidParam);
        }

        QRegularExpression pwdRegex("^(?=.*[a-zA-Z])(?=.*\\d).{8,16}$");
        if (!pwdRegex.match(password).hasMatch()) {
            return Result<void>::error("Password must be 8-16 characters and contain letters and numbers", ErrorCode::InvalidParam);
        }

        auto existing = m_userMapper->findByEmail(email);
        if (existing.isOk() && existing.value().has_value()) {
            return Result<void>::error("Email is already registered", ErrorCode::InvalidParam);
        }

        int role = 0;
        auto countRes = m_userMapper->countUsers();
        if (countRes.isOk() && countRes.value() == 0) {
            role = 1;
        }

        auto userRes = buildUserEntity(email, password, role);
        if (!userRes.isOk()) return Result<void>::error(userRes.errorMessage(), userRes.errorCode());
        return m_userMapper->insert(userRes.value());
    }

    Result<QJsonObject> AuthService::login(const QString& email, const QString& password) const {
        auto userRes = m_userMapper->findByEmail(email);
        if (!userRes.isOk() || !userRes.value().has_value()) {
            return Result<QJsonObject>::error("Invalid email or password", ErrorCode::AuthorizationFailed);
        }

        const auto& user = userRes.value().value();

        if (user.status == 0) {
            return Result<QJsonObject>::error("Account has been disabled", ErrorCode::AuthorizationFailed);
        }

        if (user.lockedUntil.isValid() && user.lockedUntil > QDateTime::currentDateTimeUtc()) {
            return Result<QJsonObject>::error("Account is temporarily locked. Please try again later.", ErrorCode::AuthorizationFailed);
        }

        if (!CryptoUtils::verifyPassword(password.toStdString(), user.passwordHash.toStdString())) {
            if (auto res = m_userMapper->incrementFailedAttempts(user.id); !res.isOk()) {
                LOG_ERROR("Database", "incrementFailedAttempts failed: " + res.errorMessage());
            }
            if (user.failedAttempts + 1 >= 5) {
                if (auto res = m_userMapper->lockAccount(user.id, 5); !res.isOk()) {
                    LOG_ERROR("Database", "lockAccount failed: " + res.errorMessage());
                }
                return Result<QJsonObject>::error("Too many failed attempts. Account locked for 5 minutes.", ErrorCode::AuthorizationFailed);
            }
            return Result<QJsonObject>::error("Invalid email or password", ErrorCode::AuthorizationFailed);
        }

        // Reset failed attempts on success
        if (user.failedAttempts > 0) {
            if (auto res = m_userMapper->resetFailedAttempts(user.id); !res.isOk()) {
                LOG_ERROR("Database", "resetFailedAttempts failed: " + res.errorMessage());
            }
        }

        QString token = JwtUtils::generateToken(user.id, user.role, user.passwordHash, m_jwtSecret);
        
        QJsonObject resData;
        resData["token"] = token;
        resData["email"] = user.email;
        resData["id"] = user.id;
        resData["role"] = user.role;

        return Result<QJsonObject>::ok(resData);
    }

    Result<void> AuthService::resetPassword(const QString& email, const QString& code, const QString& newPassword) const {
        std::string cachedCode;
        if (!utils::MemoryCache::getInstance().get("VERIFY_" + email.toStdString(), cachedCode)) {
            return Result<void>::error("Verification code expired or not found", ErrorCode::InvalidParam);
        }

        if (QString::fromStdString(cachedCode) != code) {
            return Result<void>::error("Incorrect verification code", ErrorCode::InvalidParam);
        }

        QRegularExpression pwdRegex("^(?=.*[a-zA-Z])(?=.*\\d).{8,16}$");
        if (!pwdRegex.match(newPassword).hasMatch()) {
            return Result<void>::error("Password must be 8-16 characters and contain letters and numbers", ErrorCode::InvalidParam);
        }

        auto existing = m_userMapper->findByEmail(email);
        if (!existing.isOk() || !existing.value().has_value()) {
            return Result<void>::error("User not found", ErrorCode::RecordNotFound);
        }

        std::string newHash = CryptoUtils::hashPassword(newPassword.toStdString());
        auto res = m_userMapper->updatePassword(email, QString::fromStdString(newHash));
        
        if (res.isOk()) {
            utils::MemoryCache::getInstance().remove("VERIFY_" + email.toStdString());
        }

        return res;
    }

    Result<void> AuthService::createAdmin(const QString& email, const QString& password) const {
        auto existing = m_userMapper->findByEmail(email);
        if (existing.isOk() && existing.value().has_value()) {
            return Result<void>::error("Email is already registered", ErrorCode::InvalidParam);
        }

        auto userRes = buildUserEntity(email, password, 1);
        if (!userRes.isOk()) return Result<void>::error(userRes.errorMessage(), userRes.errorCode());

        return m_userMapper->insert(userRes.value());
    }
}
