#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "Types.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QObject>
#include <functional>
#include <QMap>
#include <QString>

namespace radar::network {
    using RequestHandler = std::function<void(
        QTcpSocket* socket, const QString& path,
        const QMap<QString, QString>& params,
        const QMap<QString, QString>& headers)>;

    // Http 服务器，基于 QTcpServer 实现，支持 get 请求，用于文件下载和 API 访问。
    class HttpServer : public QTcpServer {
        Q_OBJECT
    public:
        explicit HttpServer(QObject* parent = nullptr);
        // 注册路由处理函数
        void route(const QString& path, const RequestHandler& handler);
        Result<void> start(uint16_t port);

    protected:
        void incomingConnection(qintptr socketDescriptor) override;

    private:
        QMap<QString, RequestHandler> m_routes;

        void handleClient(QTcpSocket* socket);
        void parseRequest(QTcpSocket* socket);
        static QMap<QString, QString> parseQueryParams(const QString& queryString);
    };
}

#endif // HTTP_SERVER_H
