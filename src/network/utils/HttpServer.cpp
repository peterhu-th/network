#include <QTextStream>
#include <QUrl>
#include "HttpServer.h"

namespace radar::network {
    HttpServer::HttpServer(QObject *parent) : QTcpServer(parent) {}

    Result<void> HttpServer::start(const QHostAddress& address, uint16_t port) {
        if (this->listen(address, port)) {
            qDebug() << "HttpServer started on port" << port;
            return Result<void>::ok();
        }
        qWarning() << "Failed to start HttpServer on port" << port << ": " << this->errorString();
        return Result<void>::error(this->errorString(), ErrorCode::NetworkListenFailed);
    }

    void HttpServer::route(const QString &path, const RequestHandler &handler) {
        m_routes[path] = handler;
    }

    void HttpServer::incomingConnection(qintptr socketDescriptor) {
        auto socket = new QTcpSocket(this);
        if (socket->setSocketDescriptor(socketDescriptor)) {
            handleClient(socket);
        } else {
            delete socket;
        }
    }

    void HttpServer::handleClient(QTcpSocket *socket) {
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            this->parseRequest(socket);
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    }

    void HttpServer::parseRequest(QTcpSocket *socket) {
        if (!socket->canReadLine()) return;
        // 读取请求头至空行
        QString requestLine = QString::fromUtf8(socket->readLine()).trimmed();
        QStringList parts = requestLine.split(' '); // 按空格拆分
        if (parts.size() < 2) {
            socket->disconnectFromHost();
            return;
        }
        QString& method = parts[0];
        QString& fullPath = parts[1];
        QMap<QString, QString> headers;
        while (socket->canReadLine()) {
            QString line = QString::fromUtf8(socket->readLine().trimmed());
            if (line.isEmpty()) break;
            int colonIndex = line.indexOf(':');
            if (colonIndex > 0) {
                QString key = line.left(colonIndex).trimmed().toLower();
                QString value = line.mid(colonIndex + 1).trimmed();
                headers[key] = value;
            }
        }
        // 校验请求方法，允许跨域，拦截非 OPTION 和 GET 请求
        if (method == "OPTIONS") {
            QByteArray response;
            response.append("HTTP/1.1 204 No Content\r\n");
            response.append("Access-Control-Allow-Origin: *\r\n");
            response.append("Access-Control-Allow-Methods: GET, OPTIONS\r\n");
            response.append("Access-Control-Allow-Headers: Authorization, Range\r\n\r\n");
            response.append("Connection: close\r\n\r\n");
            socket->write(response);
            socket->disconnectFromHost();
            return;
        } else if (method != "GET") {
            QTextStream os(socket);
            QByteArray response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
            socket->write(response);
            socket->disconnectFromHost();
            return;
        }
        QUrl url(fullPath);
        QString path = url.path();
        QMap<QString, QString> params;
        QString query = url.query();
        if (!query.isEmpty()) {
            params = parseQueryParams(query);
        }
        // 路由分发
        bool handled = false;
        for (auto it = m_routes.begin(); it != m_routes.end(); ++it) {
            if (path.startsWith(it.key())) {
                it.value()(socket, path, params, headers);
                handled = true;
                break;
            }
        }

        if (!handled) {
            QByteArray response = "HTTP/1.1 404 Not Found\r\n\r\n";
            socket->write(response);
            socket->disconnectFromHost();
        }
    }


    QMap<QString, QString> HttpServer::parseQueryParams(const QString &queryString) {
        QMap<QString, QString> params;
        QStringList pairs = queryString.split('&');
        for (const QString& pair : pairs) {
            QStringList kv =pair.split('=');
            if (kv.size() == 2) {
                params.insert(QUrl::fromPercentEncoding(kv[0].toUtf8()),
                    QUrl::fromPercentEncoding(kv[1].toUtf8()));
            }
        }
        return params;
    }
}
