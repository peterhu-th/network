#include "AudioRecordController.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileInfo>
#include <QDebug>
#include <QFile>
#include <algorithm>

namespace radar::network::controller {

    AudioRecordController::AudioRecordController(QObject* parent) : QObject(parent) {
        m_service = std::make_unique<service::AudioRecordService>();
        m_httpServer = std::make_unique<HttpServer>();
    }

    AudioRecordController::~AudioRecordController() {
        stop();
    }

    Result<void> AudioRecordController::init(const QVariantMap& config) {
        DatabaseConfig dbConfig;
        if (config.contains("database")) {
            QVariantMap dbMap = config["database"].toMap();
            if (dbMap.contains("type")) dbConfig.type = dbMap["type"].toString();
            dbConfig.host = dbMap.value("host", "127.0.0.1").toString();
            dbConfig.port = dbMap.value("port", 5432).toInt();
            dbConfig.username = dbMap.value("user", "postgres").toString();
            dbConfig.password = dbMap.value("pass", "").toString();
            dbConfig.dbName = dbMap.value("name", "circle").toString(); // 默认 circle 数据库
        }

        QString storagePath = ".";
        if (config.contains("storage")) {
            storagePath = config["storage"].toMap().value("path", ".").toString();
        }

        if (config.contains("network")) {
            m_port = config["network"].toMap().value("port", 8080).toInt();
        }

        auto res = m_service->init(dbConfig, storagePath);
        if (!res.isOk()) {
            return Result<void>::error("Service init failed: " + res.errorMessage(), res.errorCode());
        }

        setupRoutes();
        return Result<void>::ok();
    }

    Result<void> AudioRecordController::start() {
        m_service->start();
        auto res = m_httpServer->start(m_port);
        if (!res.isOk()) {
            return Result<void>::error("HttpServer error: " + res.errorMessage(), res.errorCode());
        }
        return Result<void>::ok();
    }

    void AudioRecordController::stop() {
        if (m_httpServer) m_httpServer->close();
        if (m_service) m_service->stop();
    }

    void AudioRecordController::setupRoutes() {
        m_httpServer->route("/api/files", [this](QTcpSocket* s, const QString&, const QMap<QString, QString>& p, const QMap<QString, QString>& h) {
            this->handleListFiles(s, p, h);
        });

        m_httpServer->route("/api/download", [this](QTcpSocket* s, const QString& path, const QMap<QString, QString>&, const QMap<QString, QString>& h) {
            this->handleDownload(s, path, h);
        });
    }

    void AudioRecordController::handleListFiles(QTcpSocket* socket, const QMap<QString, QString>& params, const QMap<QString, QString>& headers) {
        QDateTime startTime = QDateTime::fromMSecsSinceEpoch(0);
        QDateTime endTime = QDateTime::currentDateTime().addDays(1);
        int limit = 100;
        int offset = 0;

        if (params.contains("startTime")) {
            startTime = QDateTime::fromMSecsSinceEpoch(params["startTime"].toLongLong());
        }
        if (params.contains("endTime")) {
            endTime = QDateTime::fromMSecsSinceEpoch(params["endTime"].toLongLong());
        }
        if (params.contains("limit")) limit = params["limit"].toInt();
        if (params.contains("offset")) offset = params["offset"].toInt();

        // 获取当前页数据
        auto result = m_service->getRecordsPage(startTime, endTime, limit, offset);
        if (!result.isOk()) {
            socket->write("HTTP/1.1 500 Internal Server Error\r\n\r\n");
            socket->disconnectFromHost();
            return;
        }

        // 获取总数
        auto countResult = m_service->getTotalCount(startTime, endTime);
        int totalCount = countResult.isOk() ? countResult.value() : 0;

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
        resp["total"] = totalCount;
        resp["limit"] = limit;
        resp["offset"] = offset;

        QByteArray data = QJsonDocument(resp).toJson();

        socket->write("HTTP/1.1 200 OK\r\n");
        socket->write("Content-Type: application/json\r\n");
        socket->write("Content-Length: " + QByteArray::number(data.size()) + "\r\n");
        socket->write("Access-Control-Allow-Origin: *\r\n");
        socket->write("\r\n");
        socket->write(data);
        socket->disconnectFromHost();
    }

    void AudioRecordController::handleDownload(QTcpSocket* socket, const QString& path, const QMap<QString, QString>& headers) {
        QStringList parts = path.split('/');
        if (parts.isEmpty()) {
            socket->write("HTTP/1.1 400 Bad Request\r\n\r\n");
            socket->disconnectFromHost();
            return;
        }

        QString idStr = parts.last();
        bool ok;
        int64_t id = idStr.toLongLong(&ok);

        if (!ok) {
            socket->write("HTTP/1.1 400 Bad Request\r\n\r\n");
            socket->disconnectFromHost();
            return;
        }

        auto result = m_service->getRecordById(id);
        if (!result.isOk()) {
            socket->write("HTTP/1.1 404 Not Found\r\n\r\n");
            socket->disconnectFromHost();
            return;
        }

        auto file = new QFile(result.value().filePath, socket);
        if (!file->open(QIODevice::ReadOnly)) {
            socket->write("HTTP/1.1 500 File Open Failed\r\n\r\n");
            socket->disconnectFromHost();
            return;
        }

        qint64 fileSize = file->size();
        socket->write("HTTP/1.1 200 OK\r\n");
        socket->write("Content-Type: audio/wav\r\n");
        socket->write(QString("Content-Length: %1\r\n").arg(fileSize).toUtf8());
        socket->write("Content-Disposition: attachment; filename=\"download.wav\"\r\n");
        socket->write("Access-Control-Allow-Origin: *\r\n");
        socket->write("\r\n");

        auto remainingBytes = std::make_shared<qint64>(fileSize);

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
    }
}
