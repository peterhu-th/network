#ifndef AUDIO_RECORD_CONTROLLER_H
#define AUDIO_RECORD_CONTROLLER_H

#include <QObject>
#include <memory>
#include "../service/AudioRecordService.h"
#include "../utils/HttpServer.h"

// 自动扫描本地音频文件并提取数据，生成唯一 ID ，保存数据到 PostgreSQL 数据库中，通过 HTTP 服务器提供前端接口
//
namespace radar::network {
    class AudioRecordController : public QObject {
        Q_OBJECT
    public:
        explicit AudioRecordController(QObject *parent = nullptr);
        ~AudioRecordController() override;
        [[nodiscard]] Result<void> init(const QVariantMap& config);
        [[nodiscard]] Result<void> start();
        void stop();
    private:
        std::unique_ptr<AudioRecordService> m_service;
        std::unique_ptr<HttpServer> m_httpServer;
        int m_port = 8080;
        void setupRoutes();
        static bool checkAuthorization(QTcpSocket* socket, const QMap<QString, QString>& params, const QMap<QString, QString>& headers);
        void handleListFiles(QTcpSocket* socket, const QMap<QString, QString>& params, const QMap<QString, QString>& headers);
        void handleDownload(QTcpSocket* socket, const QString& path, const QMap<QString, QString>& params, const QMap<QString, QString>& headers);
    };
}

#endif
