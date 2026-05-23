#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include <QString>
#include <QJsonObject>
#include <memory>
#include "../../core/Types.h"
#include "../mapper/UserMapper.h"
#include "../utils/SmtpClient.h"

namespace radar::network {
    class AuthService {
    public:
        AuthService(std::shared_ptr<UserMapper> userMapper, std::shared_ptr<SmtpClient> smtpClient, QString jwtSecret);
        ~AuthService() = default;

        [[nodiscard]] Result<void> sendVerificationCode(const QString& email) const;
        [[nodiscard]] Result<void> registerUser(const QString& email, const QString& code, const QString& password) const;
        [[nodiscard]] Result<QJsonObject> login(const QString& email, const QString& password) const;
        
        // 生成管理员，如果系统无用户
        [[nodiscard]] Result<void> initSystemAdmin(const QString& adminEmail, const QString& adminPassword) const;
        
        // 创建管理员（必须已有管理员权限才能调用）
        [[nodiscard]] Result<void> createAdmin(const QString& email, const QString& password) const;

    private:
        std::shared_ptr<UserMapper> m_userMapper;
        std::shared_ptr<SmtpClient> m_smtpClient;
        QString m_jwtSecret;
    };
}

#endif // AUTH_SERVICE_H
