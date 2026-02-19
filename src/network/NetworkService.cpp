#include "NetworkService.h"
#include "Config.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileInfo>
#include <QDebug>
#include <QFile>
#include <algorithm>

namespace radar::network {
    NetworkService::NetworkService(QObject* parent) : QObject(parent) {
        m_dbManager = std::make_unique<DatabaseManager>();
        m_httpServer = std::make_unique<HttpServer>();
        m_fileIndexer = std::make_unique<FileIndexer>(m_dbManager.get());
    }

    NetworkService::~NetworkService() {
        stop();
    }

    Result<void> NetworkService::init(const QVariantMap& config) {
        // 1. Init Database
        if (!config.contains("database")) {
            return Result<void>::error("Missing 'database' config", ErrorCode::InvalidConfig);
        }
        DatabaseConfig dbConfig;
        if (config.contains("database")) {
            QVariantMap dbMap = config["database"].toMap();
            if (dbMap.contains("type")) {
                dbConfig.type = dbMap["type"].toString();
            }
            dbConfig.host = dbMap.value("host", "127.0.0.1").toString();
            dbConfig.port = dbMap.value("port", 3306).toInt();
            dbConfig.username = dbMap.value("user", "root").toString();
            dbConfig.password = dbMap.value("pass", "").toString();
            dbConfig.dbName = dbMap.value("name", "audio_radar").toString();
        }
        if (auto res = m_dbManager->init(dbConfig); !res.isOk()) {
            qWarning() << "Failed to init database:" << res.errorMessage();
            return Result<void>::error(res.errorMessage(), ErrorCode::DatabaseInitFailed);
        }

        // 2. Config other services
        if (config.contains("network")) {
            m_port = config["network"].toMap().value("port", 8080).toInt();
        }
        if (config.contains("storage")) {
            m_storagePath = config["storage"].toMap().value("path", ".").toString();
        }
        setupRoutes();
        return Result<void>::ok();
    }

    Result<void> NetworkService::start() const {
        // Start Indexer
        if (auto res = m_fileIndexer->start(m_storagePath); !res.isOk()) {
            return Result<void>::error("Failed to start FileIndexer: " + res.errorMessage(), res.errorCode());
        }

        // Start HTTP Server
        if (auto res = m_httpServer->start(m_port); !res.isOk()) {
            return Result<void>::error("Failed to start HTTP server: ", ErrorCode::HttpServerError);
        }
        return Result<void>::ok();
    }

    void NetworkService::stop() const {
        m_httpServer->close();
    }

    void NetworkService::setupRoutes() const {
        m_httpServer->route("/api/files", [this](QTcpSocket* s, const QString&, const QMap<QString, QString>& p, const QMap<QString, QString>& h) {
            auto res = this->handleListFiles(s, p, h);
            if (!res.isOk()) {
                qWarning() << "handleListFiles failed:" << res.errorMessage();
            }
        });

        // 假设下载路径为 /api/download/<id>；假设 HttpServer::route 是前缀匹配
        m_httpServer->route("/api/download", [this](QTcpSocket* s, const QString& path, const QMap<QString, QString>&, const QMap<QString, QString>& h) {
            auto res = this->handleDownload(s, path, h);
            if (!res.isOk()) {
                 qWarning() << "handleDownload failed:" << res.errorMessage();
            }
        });
    }

    Result<void> NetworkService::handleListFiles(QTcpSocket* socket, const QMap<QString, QString>& params, const QMap<QString, QString>& headers) const {
        QDateTime startTime = QDateTime::fromMSecsSinceEpoch(56);
        QDateTime endTime = QDateTime::currentDateTime().addDays(1);
        int limit = 100;

        if (params.contains("startTime")) {
            startTime = QDateTime::fromMSecsSinceEpoch(params["stareTime"].toLongLong());
        }
        if (params.contains("endTime")) {
            endTime = QDateTime::fromMSecsSinceEpoch(params["endTime"].toLongLong());
        }
        if (params.contains("limit")) limit = params["limit"].toInt();
        // TODO: parsing time params

        auto result = m_dbManager->queryRecords(startTime, endTime, limit); // fetch all for now

        if (!result.isOk()) {
            QTextStream os(socket);
            os << "HTTP/1.1 500 Internal Server Error\r\n\r\n";
            socket->disconnectFromHost();
            return Result<void>::error(result.errorMessage(), result.errorCode());
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
        return Result<void>::ok();
    }

    Result<void> NetworkService::handleDownload(QTcpSocket* socket, const QString& path, const QMap<QString, QString>& headers) const {
        // 鉴权
        if (headers.value("authorization") != "Bearer my_secret_token") {
            socket->write("HTTP/1.1 401 Unauthorized\r\n");
            socket->write("WWW-Authenticate: Bearer realm=\"AudioRadar\"\r\n\r\n");
            socket->disconnectFromHost();
            return Result<void>::error("Unauthorized request", ErrorCode::InvalidConfig);
        }
        // 解析 ID 并获取文件信息
        // path e.g. /api/download/12345
        QStringList parts = path.split('/');
        if (parts.isEmpty()) return Result<void>::error("Invalid path", ErrorCode::InvalidConfig);

        QString& idStr = parts.last();
        bool ok;
        int64_t id = idStr.toLongLong(&ok);

        if (!ok) {
            socket->write("HTTP/1.1 400 Bad Request\r\n\r\n");
            socket->disconnectFromHost();
            return Result<void>::error("Invalid ID format", ErrorCode::InvalidConfig);
        }

        auto result = m_dbManager->getRecord(id);
        if (!result.isOk()) {
            socket->write("HTTP/1.1 404 Not Found\r\n\r\n");
            socket->disconnectFromHost();
            return Result<void>::error("Record not found", ErrorCode::DatabaseQueryFailed);
        }

        auto file = new QFile(result.value().filePath, socket);

        if (!file->open(QIODevice::ReadOnly)) {
            socket->write("HTTP/1.1 500 File Open Failed\r\n\r\n");
            socket->disconnectFromHost();
            return Result<void>::error("File open failed", ErrorCode::NetworkFileIoFailed); // Using NetworkFileIoFailed as generic IO error
        }
        // 断点续传
        qint64 fileSize = file->size();
        qint64 startPos = 0;
        qint64 endPos = fileSize - 1;
        bool isPartial = false;

        QString rangeHeader = headers.value("range");
        if (rangeHeader.startsWith("bytes=")) {
            QString range = rangeHeader.mid(6);
            QStringList rangeParts = range.split('-');
            if (rangeParts.size() == 2) {
                if (!rangeParts[0].isEmpty()) startPos = rangeParts[0].toLongLong();
                if (!rangeParts[1].isEmpty()) endPos = rangeParts[1].toLongLong();
                isPartial = true;
            }
        }

        if (startPos > endPos || startPos >= fileSize) {
            socket->write("HTTP/1.1 416 Requested Range Not Satisfiable\r\n");
            socket->write(QString("Content-Range: bytes */%1\r\n\r\n").arg(fileSize).toUtf8());
            socket->disconnectFromHost();
            return Result<void>::error("Invalid range requested", ErrorCode::InvalidConfig);
        }
        file->seek(startPos);
        qint64 contentLength = endPos - startPos + 1;

        if (isPartial) {
            socket->write("HTTP/1.1 206 Partial Content\r\n");
            socket->write(QString("Content-Range: bytes %1-%2/%3\r\n").arg(startPos).arg(endPos).arg(fileSize).toUtf8());
        } else {
            socket->write("HTTP/1.1 200 OK\r\n");
        }
        socket->write("Content-Type: audio/wav\r\n");
        socket->write("Content-Disposition: attachment; filename=\"download.wav\"\r\n");
        socket->write("\r\n");

        // 异步流式发送
        auto remainingBytes = std::make_shared<qint64>(contentLength);

        auto sendNextChunk = [socket, file, remainingBytes]() {
            if (!socket || !file || *remainingBytes <= 0) return;
            qint64 chunkSize = std::min(*remainingBytes, static_cast<qint64>(65536));
            QByteArray chunk = file->read(chunkSize);

            if (chunk.isEmpty()) {
                socket->disconnectFromHost();
                return;
            }

            socket->write(chunk);
            *remainingBytes -= chunk.size();

            if (*remainingBytes <= 0) {
                socket->disconnectFromHost();
            }
        };

        connect(socket, &QTcpSocket::bytesWritten, socket, sendNextChunk);
        sendNextChunk();

        return Result<void>::ok();
    }
}
