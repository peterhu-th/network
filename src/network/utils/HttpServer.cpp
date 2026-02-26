#include "HttpServer.h"
#include <QTextStream>
#include <QUrl>
#include <QDebug>

namespace radar::network {
    HttpServer::HttpServer(QObject* parent) : QTcpServer(parent) {}

    Result<void> HttpServer::start(uint16_t port) {
        if (this->listen(QHostAddress::Any, port)) {
            qDebug() << "HttpServer started on port" << port;
            return Result<void>::ok();
        }
        qCritical() << "HttpServer failed to start on port" << port << ":" << this->errorString();
        return Result<void>::error(this->errorString(), ErrorCode::NetworkListenFailed);
    }

    void HttpServer::route(const QString& path, const RequestHandler& handler) {
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

    void HttpServer::handleClient(QTcpSocket* socket) {
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            this->parseRequest(socket);
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    }

    void HttpServer::parseRequest(QTcpSocket* socket) {
        if (!socket->canReadLine()) return;

        QString requestLine = QString::fromUtf8(socket->readLine()).trimmed();
        QStringList parts = requestLine.split(' ');

        if (parts.size() < 2) {
            socket->disconnectFromHost();
            return;
        }

        QString& method = parts[0];
        QString& fullPath = parts[1];

        if (method != "GET") {
            // Only GET supported for now
            QTextStream os(socket);
            os.setAutoDetectUnicode(true);
            os << "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
            socket->disconnectFromHost();
            return;
        }

        QMap<QString, QString> headers;
        while (socket->canReadLine()) {
            QString line = QString::fromUtf8(socket->readLine()).trimmed();
            if (line.isEmpty()) break;

            int colonIdx = line.indexOf(':');
            if (colonIdx > 0) {
                QString key = line.left(colonIdx).trimmed().toLower();
                QString value = line.mid(colonIdx + 1).trimmed();
                headers[key] = value;
            }
        }

        // Parse path and query params
        QUrl url(fullPath);
        QString path = url.path();

        // Simple QMap for params (Qt5 QUrlQuery usage)
        QMap<QString, QString> params;
        QString query = url.query();
        if (!query.isEmpty()) {
            params = parseQueryParams(query);
        }

        // Find handler
        bool handled = false;
        for (auto it = m_routes.begin(); it != m_routes.end(); ++it) {
            if (path.startsWith(it.key())) {
                it.value()(socket, path, params, headers);
                handled = true;
                break;
            }
        }

        if (!handled) {
            QTextStream os(socket);
            os.setAutoDetectUnicode(true);
            os << "HTTP/1.1 404 Not Found\r\n\r\n";
            socket->disconnectFromHost();
        }
    }

    QMap<QString, QString> HttpServer::parseQueryParams(const QString& queryString) {
        QMap<QString, QString> params;
        QStringList pairs = queryString.split('&');
        for (const QString& pair : pairs) {
            QStringList kv = pair.split('=');
            if (kv.size() == 2) {
                params.insert(QUrl::fromPercentEncoding(kv[0].toUtf8()),
                              QUrl::fromPercentEncoding(kv[1].toUtf8()));
            }
        }
        return params;
    }
}
