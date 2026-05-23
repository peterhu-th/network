#ifndef SMTP_CLIENT_H
#define SMTP_CLIENT_H

#include <string>
#include "../../core/Types.h"

namespace radar::network {
    class SmtpClient {
    public:
        SmtpClient(std::string smtpServer, int port, std::string username, std::string password);
        ~SmtpClient() = default;

        // 发送验证码邮件
        [[nodiscard]] Result<void> sendEmail(const std::string& to, const std::string& subject, const std::string& body) const;

    private:
        std::string m_smtpServer;
        int m_port;
        std::string m_username;
        std::string m_password;
    };
}

#endif // SMTP_CLIENT_H
