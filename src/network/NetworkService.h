#ifndef NETWORK_SERVICE_H
#define NETWORK_SERVICE_H

#include "DatabaseManager.h"
#include "HttpServer.h"
#include "FileIndexer.h"
#include <QObject>
#include <memory>

namespace radar {
namespace network {

/**
 * @brief 网络服务主控类
 * @details 协调数据库、文件索引和 HTTP 服务。
 */
class NetworkService : public QObject {
    Q_OBJECT
public:
    explicit NetworkService(QObject* parent = nullptr);
    ~NetworkService();

    /**
     * @brief 初始化服务
     * @param config 配置 (包含 db, network, storage 路径等)
     * @return bool
     */
    bool init(const QVariantMap& config);

    /**
     * @brief 启动服务
     * @return bool
     */
    bool start();

    /**
     * @brief 停止服务
     */
    void stop();

private:
    std::unique_ptr<DatabaseManager> m_dbManager;
    std::unique_ptr<HttpServer> m_httpServer;
    std::unique_ptr<FileIndexer> m_fileIndexer;
    
    QString m_storagePath;
    int m_port = 8080;

    void setupRoutes();
    void handleListFiles(QTcpSocket* socket, const QMap<QString, QString>& params);
    void handleDownload(QTcpSocket* socket, const QString& path);
};

} // namespace network
} // namespace radar

#endif // NETWORK_SERVICE_H
