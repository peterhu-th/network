#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QObject>
#include <functional>
#include <QMap>

namespace radar {
namespace network {

using RequestHandler = std::function<void(QTcpSocket*, const QString& path, const QMap<QString, QString>& params)>;

/**
 * @brief 简易 HTTP 服务器
 * @details 基于 QTcpServer 实现，支持 GET 请求，用于文件下载和 API 访问。
 */
class HttpServer : public QTcpServer {
    Q_OBJECT
public:
    explicit HttpServer(QObject* parent = nullptr);

    /**
     * @brief 注册路由处理函数
     * @param path 路径前缀 (如 "/api/files")
     * @param handler 处理函数
     */
    void route(const QString& path, RequestHandler handler);

    /**
     * @brief 启动服务器
     * @param port 端口好
     * @return bool 是否成功
     */
    bool start(uint16_t port);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    QMap<QString, RequestHandler> m_routes;
    
    void handleClient(QTcpSocket* socket);
    void parseRequest(QTcpSocket* socket);
    QMap<QString, QString> parseQueryParams(const QString& queryString);
};

} // namespace network
} // namespace radar

#endif // HTTP_SERVER_H
