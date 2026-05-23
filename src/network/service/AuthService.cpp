#include "AuthService.h"
#include "../utils/MemoryCache.h"
#include "../utils/CryptoUtils.h"
#include "../utils/JwtUtils.h"
#include "../utils/IdGenerator.h"
#include <QRandomGenerator>
#include <QDateTime>

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
        user.createdAt = QDateTime::currentDateTime();
        user.updatedAt = QDateTime::currentDateTime();
        return Result<UserEntity>::ok(user);
    }

    Result<void> AuthService::sendVerificationCode(const QString& email) const {
        // 生成 6 位随机数
        int codeInt = QRandomGenerator::global()->bounded(100000, 1000000);
        std::string codeStr = std::to_string(codeInt);

        // 存入缓存 5分钟 (300秒)
        utils::MemoryCache::getInstance().set("VERIFY_" + email.toStdString(), codeStr, 300);

        std::string subject = "Audio Radar Registration Code";
        std::string body = "Your verification code is: " + codeStr + "\nThis code will expire in 5 minutes.";

        return m_smtpClient->sendEmail(email.toStdString(), subject, body);
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
        auto existing = m_userMapper->findByEmail(email);
        if (existing.isOk() && existing.value().has_value()) {
            return Result<void>::error("Email is already registered", ErrorCode::InvalidParam);
        }
        auto userRes = buildUserEntity(email, password, 0);
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

        if (!CryptoUtils::verifyPassword(password.toStdString(), user.passwordHash.toStdString())) {
            return Result<QJsonObject>::error("Invalid email or password", ErrorCode::AuthorizationFailed);
        }

        QString token = JwtUtils::generateToken(user.id, user.role, m_jwtSecret);
        
        QJsonObject resData;
        resData["token"] = token;
        resData["email"] = user.email;
        resData["id"] = user.id;
        resData["role"] = user.role;

        return Result<QJsonObject>::ok(resData);
    }

    Result<void> AuthService::initSystemAdmin(const QString& adminEmail, const QString& adminPassword) const {
        auto countRes = m_userMapper->countUsers();
        if (!countRes.isOk()) {
            return Result<void>::error("Failed to check user count", countRes.errorCode());
        }

        if (countRes.value() > 0) return Result<void>::ok();

        auto userRes = buildUserEntity(adminEmail, adminPassword, 1);
        if (!userRes.isOk()) return Result<void>::error(userRes.errorMessage(), userRes.errorCode());

        return m_userMapper->insert(userRes.value());
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
