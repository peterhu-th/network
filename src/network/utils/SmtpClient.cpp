#include "SmtpClient.h"
#include <curl/curl.h>
#include <cstring>
#include <QtGlobal>
#include "../../core/Logger.h"

namespace radar::network {

    struct EmailData {
        const char* text;
        size_t bytesLeft;
    };

    static size_t payloadSource(char* ptr, size_t size, size_t nmemb, void* userp) {
        auto* data = static_cast<EmailData*>(userp);
        if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1)) {
            return 0;
        }

        size_t bytesToCopy = std::min(size * nmemb, data->bytesLeft);
        if (bytesToCopy > 0) {
            std::memcpy(ptr, data->text, bytesToCopy);
            data->text += bytesToCopy;
            data->bytesLeft -= bytesToCopy;
            return bytesToCopy;
        }

        return 0; // EOF
    }

    SmtpClient::SmtpClient(std::string smtpServer, int port, std::string username, std::string password)
        : m_smtpServer(std::move(smtpServer)), m_port(port), m_username(std::move(username)), m_password(std::move(password)) {
    }

    Result<void> SmtpClient::sendEmail(const std::string& to, const std::string& subject, const std::string& body) const {
        CURL* curl = curl_easy_init();
        if (!curl) {
            return Result<void>::error(QString("Failed to initialize libcurl"), ErrorCode::ToolsError);
        }

        // 1. 动态判断协议前缀：465 端口使用隐式 smtps，其他端口（如 587/25）使用 smtp
        std::string protocol = (m_port == 465) ? "smtps://" : "smtp://";
        std::string url = protocol + m_smtpServer + ":" + std::to_string(m_port);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // 2. 如果是 smtp:// 协议（非 465），强制要求服务器开启 TLS (STARTTLS)
        if (m_port != 465) {
            curl_easy_setopt(curl, CURLOPT_USE_SSL, static_cast<long>(CURLUSESSL_ALL));
        }

        curl_easy_setopt(curl, CURLOPT_USERNAME, m_username.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, m_password.c_str());

        curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, "AUTH=LOGIN");
        if (qEnvironmentVariableIntValue("AUDIO_SMTP_INSECURE") == 1) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }

        std::string mailFrom = "<" + m_username + ">";
        std::string mailTo = "<" + to + ">";

        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, m_username.c_str());
        struct curl_slist* recipients = nullptr;
        recipients = curl_slist_append(recipients, to.c_str());
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        std::string payload = "To: " + to + "\r\n"
                              "From: Audio Radar <" + m_username + ">\r\n"
                              "Subject: " + subject + "\r\n"
                              "\r\n" + body + "\r\n";

        EmailData data;
        data.text = payload.c_str();
        data.bytesLeft = payload.length();

        curl_easy_setopt(curl, CURLOPT_READFUNCTION, payloadSource);
        curl_easy_setopt(curl, CURLOPT_READDATA, &data);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            return Result<void>::error(QString("Failed to send email: ") + QString(curl_easy_strerror(res)), ErrorCode::NetworkTimeout);
        }

        return Result<void>::ok();
    }
}
