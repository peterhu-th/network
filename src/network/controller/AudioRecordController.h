#ifndef AUDIO_RECORD_CONTROLLER_H
#define AUDIO_RECORD_CONTROLLER_H

#include "../service/AudioRecordService.h"
#include "../utils/HttpServer.h"
#include <QObject>
#include <memory>

namespace radar::network::controller {

    class AudioRecordController : public QObject {
        Q_OBJECT
    public:
        explicit AudioRecordController(QObject* parent = nullptr);
        ~AudioRecordController() override;

        Result<void> init(const QVariantMap& config);
        Result<void> start();
        void stop();

    private:
        std::unique_ptr<service::AudioRecordService> m_service;
        std::unique_ptr<HttpServer> m_httpServer;
        int m_port = 8080;

        void setupRoutes();
        void handleListFiles(QTcpSocket* socket, const QMap<QString, QString>& params, const QMap<QString, QString>& headers);
        void handleDownload(QTcpSocket* socket, const QString& path, const QMap<QString, QString>& headers);
    };

}

#endif // AUDIO_RECORD_CONTROLLER_H
