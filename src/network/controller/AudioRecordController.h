#ifndef AUDIO_RECORD_CONTROLLER_H
#define AUDIO_RECORD_CONTROLLER_H

#include <memory>
#include <QHttpServer>
#include <QHostAddress>
#include <QHttpServerResponder>
#include <QHttpServerResponse>
#include "../service/AudioRecordService.h"
#include "../NetworkDTO.h"

namespace radar::network {
    class AudioRecordController : public QObject {
        Q_OBJECT

    public:
        explicit AudioRecordController(QObject *parent = nullptr);
        ~AudioRecordController() override;
        Result<void> init(const DatabaseConfig& dbConfig, const NetworkConfig& netConfig);
        [[nodiscard]] Result<void> start() const;
        void stop();

    private:
        std::unique_ptr<AudioRecordService> m_service;
        std::unique_ptr<QHttpServer> m_httpServer;
        int m_port = 8080;
        QHostAddress m_bindAddress = QHostAddress::LocalHost;
        QString m_jwtSecret;

        void setupRoutes() const;
        [[nodiscard]] QHttpServerResponse handleLogin(const QHttpServerRequest& request) const;
        [[nodiscard]] Result<qint64> checkAuth(const QHttpServerRequest& request) const;
        [[nodiscard]] QHttpServerResponse handleListFiles(const QHttpServerRequest& request) const;
        void handleDownload(const QHttpServerRequest& request, QHttpServerResponder& responder) const;
    };
}
#endif
