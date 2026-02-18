#include "HttpServer.h"
#include <QTextStream>
#include <QUrl>
#include <QDateTime>
#include <QDebug>

namespace radar {
namespace network {

HttpServer::HttpServer(QObject* parent) : QTcpServer(parent) {}

bool HttpServer::start(uint16_t port) {
    if (this->listen(QHostAddress::Any, port)) {
        qDebug() << "HttpServer started on port" << port;
        return true;
    }
    qCritical() << "HttpServer failed to start on port" << port << ":" << this->errorString();
    return false;
}

void HttpServer::route(const QString& path, RequestHandler handler) {
    m_routes[path] = handler;
}

void HttpServer::incomingConnection(qintptr socketDescriptor) {
    QTcpSocket* socket = new QTcpSocket(this);
    if (socket->setSocketDescriptor(socketDescriptor)) {
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            this->parseRequest(socket);
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    } else {
        delete socket;
    }
}

void HttpServer::parseRequest(QTcpSocket* socket) {
    if (!socket->canReadLine()) return;

    QString requestLine = QString::fromUtf8(socket->readLine()).trimmed();
    QStringList parts = requestLine.split(' ');

    if (parts.size() < 2) {
        socket->disconnectFromHost();
        return;
    }

    QString method = parts[0];
    QString fullPath = parts[1];

    if (method != "GET") {
        // Only GET supported for now
        QTextStream os(socket);
        os.setAutoDetectUnicode(true);
        os << "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
        socket->disconnectFromHost();
        return;
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
            it.value()(socket, path, params);
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

} // namespace network
} // namespace radar
