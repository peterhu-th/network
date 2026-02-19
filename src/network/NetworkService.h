#ifndef NETWORK_SERVICE_H
#define NETWORK_SERVICE_H

#include "DatabaseManager.h"
#include "Types.h"
#include "HttpServer.h"
#include "FileIndexer.h"
#include <QObject>
#include <memory>
#include <QDateTime>

namespace radar::network {
    using RequestHandler = std::function<void(QTcpSocket*, const QString& path,
        const QMap<QString, QString>& params,
        const QMap<QString, QString>& headers)>;
    // 网络服务主控类：协调数据库、文件索引和 HTTP 服务
    class NetworkService : public QObject {
        Q_OBJECT
    public:
        explicit NetworkService(QObject* parent = nullptr);
        ~NetworkService() override;

        Result<void> init(const QVariantMap& config);
        [[nodiscard]] Result<void> start() const;
        void stop() const;

    private:
        std::unique_ptr<DatabaseManager> m_dbManager;
        std::unique_ptr<HttpServer> m_httpServer;
        std::unique_ptr<FileIndexer> m_fileIndexer;

        QString m_storagePath;
        int m_port = 8080;

        void setupRoutes() const;
        Result<void> handleListFiles(QTcpSocket* socket, const QMap<QString, QString>& params, const QMap<QString, QString>& headers) const;
        Result<void> handleDownload(QTcpSocket* socket, const QString& path, const QMap<QString, QString>& headers) const;
    };
}

#endif // NETWORK_SERVICE_H
