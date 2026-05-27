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
        
        // 重置密码
        [[nodiscard]] Result<void> resetPassword(const QString& email, const QString& code, const QString& newPassword) const;
        
        // 创建管理员
        [[nodiscard]] Result<void> createAdmin(const QString& email, const QString& password) const;

    private:
        std::shared_ptr<UserMapper> m_userMapper;
        std::shared_ptr<SmtpClient> m_smtpClient;
        QString m_jwtSecret;
    };
}

#endif // AUTH_SERVICE_H
