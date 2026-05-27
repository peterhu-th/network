#include "SmtpClient.h"
#include <curl/curl.h>
#include <cstring>
#include <QtGlobal>
#include <QDateTime>
#include <QUuid>
#include "../../core/Logger.h"

namespace radar::network {

    struct EmailData {
        const char* text;
        size_t bytesLeft;
    };

    static int curlDebugCallback(CURL* handle, curl_infotype type, char* data, size_t size, void* userp) {
        (void)handle;
        (void)userp;
        if (type == CURLINFO_TEXT || type == CURLINFO_HEADER_IN) {
            std::string text(data, size);
            while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
                text.pop_back();
            }
            if (!text.empty()) {
                LOG_INFO("SmtpClient", QString("CURL: ") + QString::fromStdString(text));
            }
        }
        return 0;
    }

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

        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, curlDebugCallback);

        std::string mailFrom = "<" + m_username + ">";
        std::string mailTo = "<" + to + ">";

        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, mailFrom.c_str());
        struct curl_slist* recipients = nullptr;
        recipients = curl_slist_append(recipients, mailTo.c_str());
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        QString dateStr = QDateTime::currentDateTimeUtc().toString("ddd, dd MMM yyyy HH:mm:ss +0000");
        QString senderDomain = QString::fromStdString(m_username).split("@").last();
        if (senderDomain.isEmpty()) senderDomain = "audio-radar.local";
        QString messageId = "<" + QUuid::createUuid().toString(QUuid::WithoutBraces) + "@" + senderDomain + ">";

        std::string payload = "Date: " + dateStr.toStdString() + "\r\n"
                              "Message-ID: " + messageId.toStdString() + "\r\n"
                              "To: " + to + "\r\n"
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
