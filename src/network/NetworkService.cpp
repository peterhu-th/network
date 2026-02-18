#include "NetworkService.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

namespace radar {
namespace network {

NetworkService::NetworkService(QObject* parent) : QObject(parent) {
    m_dbManager = std::make_unique<DatabaseManager>();
    m_httpServer = std::make_unique<HttpServer>();
    m_fileIndexer = std::make_unique<FileIndexer>(m_dbManager.get());
}

NetworkService::~NetworkService() {
    stop();
}

bool NetworkService::init(const QVariantMap& config) {
    // 1. Init Database
    DatabaseConfig dbConfig;
    if (config.contains("db")) {
        QVariantMap dbMap = config["db"].toMap();
        dbConfig.host = dbMap.value("host", "127.0.0.1").toString();
        dbConfig.port = dbMap.value("port", 3306).toInt();
        dbConfig.username = dbMap.value("user", "root").toString();
        dbConfig.password = dbMap.value("pass", "").toString();
        dbConfig.dbName = dbMap.value("name", "audio_radar").toString();
    }
    
    if (auto res = m_dbManager->init(dbConfig); !res.isOk()) {
        qCritical() << "Failed to init DB:" << res.errorMessage();
        return false;
    }

    // 2. Config other services
    if (config.contains("network")) {
        m_port = config["network"].toMap().value("port", 8080).toInt();
    }
    
    if (config.contains("storage")) {
        m_storagePath = config["storage"].toMap().value("path", ".").toString();
    }

    setupRoutes();
    return true;
}

bool NetworkService::start() {
    // Start Indexer
    m_fileIndexer->start(m_storagePath);

    // Start HTTP Server
    return m_httpServer->start(m_port);
}

void NetworkService::stop() {
    m_httpServer->close();
}

void NetworkService::setupRoutes() {
    m_httpServer->route("/api/files", [this](QTcpSocket* s, const QString&, const QMap<QString, QString>& p) {
        this->handleListFiles(s, p);
    });

    // 假设下载路径为 /api/download/<id>
    // 在 HttpServer 简单路由中，我们可能需要处理前缀匹配
    // 这里简单起见，HttpServer 需要支持更复杂的路由或我们在 handler 里解析
    // 这里我们在 HttpServer 里改动，或者这里注册个通用的
    // 假设 HttpServer::route 是前缀匹配
    m_httpServer->route("/api/download", [this](QTcpSocket* s, const QString& path, const QMap<QString, QString>&) {
        this->handleDownload(s, path);
    });
}

void NetworkService::handleListFiles(QTcpSocket* socket, const QMap<QString, QString>& params) {
    QDateTime start = QDateTime::currentDateTime().addDays(-30);
    QDateTime end = QDateTime::currentDateTime();
    int limit = 100;

    if (params.contains("limit")) limit = params["limit"].toInt();
    // TODO: parsing time params

    auto result = m_dbManager->queryRecords(QDateTime(), QDateTime(), limit); // fetch all for now
    
    if (!result.isOk()) {
        QTextStream os(socket);
        os << "HTTP/1.1 500 Internal Server Error\r\n\r\n";
        socket->disconnectFromHost();
        return;
    }

    QJsonArray array;
    for (const auto& record : result.value()) {
        QJsonObject obj;
        obj["id"] = QString::number(record.id);
        obj["path"] = record.filePath;
        obj["created_at"] = record.createdAt.toString(Qt::ISODate);
        obj["size"] = record.fileSize;
        array.append(obj);
    }

    QJsonObject resp;
    resp["code"] = 200;
    resp["data"] = array;

    QByteArray data = QJsonDocument(resp).toJson();
    
    // Write Response
    socket->write("HTTP/1.1 200 OK\r\n");
    socket->write("Content-Type: application/json\r\n");
    socket->write("Content-Length: " + QByteArray::number(data.size()) + "\r\n");
    socket->write("\r\n");
    socket->write(data);
    socket->disconnectFromHost();
}

void NetworkService::handleDownload(QTcpSocket* socket, const QString& path) {
    // path e.g. /api/download/12345
    QStringList parts = path.split('/');
    if (parts.isEmpty()) return;
    
    QString idStr = parts.last();
    bool ok;
    int64_t id = idStr.toLongLong(&ok);
    
    if (!ok) {
        socket->write("HTTP/1.1 400 Bad Request\r\n\r\n");
        socket->disconnectFromHost();
        return;
    }

    auto result = m_dbManager->getRecord(id);
    if (!result.isOk()) {
        socket->write("HTTP/1.1 404 Not Found\r\n\r\n");
        socket->disconnectFromHost();
        return;
    }

    QFile file(result.value().filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        socket->write("HTTP/1.1 500 File Open Failed\r\n\r\n");
        socket->disconnectFromHost();
        return;
    }

    QByteArray fileData = file.readAll();
    
    socket->write("HTTP/1.1 200 OK\r\n");
    socket->write("Content-Type: audio/wav\r\n");
    socket->write("Content-Disposition: attachment; filename=\"download.wav\"\r\n");
    socket->write("Content-Length: " + QByteArray::number(fileData.size()) + "\r\n");
    socket->write("\r\n");
    socket->write(fileData);
    socket->disconnectFromHost();
}

} // namespace network
} // namespace radar
