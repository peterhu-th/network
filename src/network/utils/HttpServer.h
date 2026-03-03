#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QString>
#include <functional>
#include "../core/Types.h"

// HTTP 服务器，监听本地端口，接收并解析前端 TCP 字节流；调用业务函数
namespace radar::network {
    // 定义函数容器
    using RequestHandler = std::function<void(
        QTcpSocket* socket,
        const QString& path,
        // Key-Value 容器，用于处理参数和请求头
        const QMap<QString, QString>& params,
        const QMap<QString, QString>& headers)>;

    class HttpServer : public QTcpServer {
        Q_OBJECT
    public:
        explicit HttpServer(QObject* parent = nullptr);
        void route(const QString& path, const RequestHandler& handler); // 注册路由
        Result<void> start(uint16_t port);
    protected:
        void incomingConnection(qintptr socketDescriptor) override;
    private:
        QMap<QString, RequestHandler> m_routes;
        void handleClient(QTcpSocket* socket);
        void parseRequest(QTcpSocket* socket);
        static QMap<QString, QString> parseQueryParams(const QString& queryString); // 映射路由表
    };
}

#endif
