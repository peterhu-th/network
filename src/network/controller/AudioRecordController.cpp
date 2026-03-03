#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QThreadPool>
#include "AudioRecordController.h"

namespace radar::network {
    AudioRecordController::AudioRecordController(QObject* parent) : QObject(parent) {
        m_service = std::make_unique<AudioRecordService>();
        m_httpServer = std::make_unique<HttpServer>();
    }

    AudioRecordController::~AudioRecordController() {
        stop();
    }

    Result<void> AudioRecordController::init(const QVariantMap& config) {
        // 提取配置
        DatabaseConfig dbConfig;
        if (config.contains("database")) {
            QVariantMap dbMap = config["database"].toMap();
            if (dbMap.contains("type")) dbConfig.type = dbMap["type"].toString();
            dbConfig.host = dbMap.value("host", "127.0.0.1").toString();
            dbConfig.port = dbMap.value("port", 5432).toInt();
            dbConfig.username = dbMap.value("user", "postgres").toString();
            dbConfig.password = dbMap.value("pass", "").toString();
            dbConfig.dbName = dbMap.value("name", "circle").toString();
        }
        QString storagePath = ".";
        if (config.contains("storage")) {
            storagePath = config["storage"].toMap().value("path", ".").toString();
        }
        if (config.contains("network")) {
            m_port = config["network"].toMap().value("port", 8080).toInt();
        }
        // 初始化 Service
        auto res = m_service->init(dbConfig, storagePath);
        if (!res.isOk()) {
            return Result<void>::error("Service init failed: " + res.errorMessage(), res.errorCode());
        }
        // 绑定 URL 路由
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

    void AudioRecordController::stop(){
        if (m_httpServer) m_httpServer->close();
        if (QThreadPool::globalInstance()) {
            QThreadPool::globalInstance()->waitForDone();
        }
        if (m_service) m_service->stop();
    }

    void AudioRecordController::setupRoutes() {
        m_httpServer->route("/api/files", [this](QTcpSocket* socket, const QString& path, const QMap<QString, QString>& params, const QMap<QString, QString>& headers) {
            this->handleListFiles(socket, params, headers);
        });

        m_httpServer->route("/api/download", [this](QTcpSocket* socket, const QString& path, const QMap<QString, QString>& params, const QMap<QString, QString>& headers) {
            this->handleDownload(socket, path, params, headers);
        });
    }

    bool AudioRecordController::checkAuthorization(QTcpSocket *socket, const QMap<QString, QString> &params, const QMap<QString, QString> &headers) {
        QString token;
        if (params.contains("token")) {
            token = params["token"];
        } else if (headers.contains("authorization")) {
            token = headers.value("authorization");
            token.replace("bearer", "", Qt::CaseInsensitive);
            token = token.trimmed();
        }
        if (token != "user") {
            QByteArray response = "HTTP/1.1 401 Unauthorized\r\n"
                                  "Access-Control-Allow-Origin: *\r\n"
                                  "Content-Type: application/json; charset=utf-8\r\n\r\n"
                                  "{\"status\":\"error\",\"message\":\"Authorization failed.\"}";
            socket->write(response);
            socket->disconnectFromHost();
            return false;
        }
        return true;
    }

    void AudioRecordController::handleListFiles(QTcpSocket* socket, const QMap<QString, QString>& params, const QMap<QString, QString>& headers) {
        if (!checkAuthorization(socket, params, headers)) return;
        // 解析参数
        int limit = params.value("limit", "20").toInt();
        int offset = params.value("offset", "0").toInt();
        QDateTime startTime;
        QDateTime endTime;
        if (params.contains("startTime")) {
            startTime = QDateTime::fromString(params["startTime"], Qt::ISODate);
        }
        if (params.contains("endTime")) {
            endTime = QDateTime::fromString(params["endTime"], Qt::ISODate);
        }

        // 调用 Service
        auto recordsRes = m_service->getRecordPage(startTime, endTime, limit, offset);
        auto countRes = m_service->getTotalCount(startTime, endTime);

        // 准备构造 JSON 响应
        QJsonObject responseObj;

        if (!recordsRes.isOk() || !countRes.isOk()) {
            responseObj["status"] = "error";
            responseObj["message"] = recordsRes.isOk() ? countRes.errorMessage() : recordsRes.errorMessage();
        } else {
            responseObj["status"] = "success";
            responseObj["total"] = countRes.value();

            QJsonArray dataArray;
            for (const auto& record : recordsRes.value()) {
                QJsonObject recordObj;
                recordObj["id"] = QString::number(record.id);
                recordObj["filePath"] = record.filePath;
                recordObj["duration"] = record.duration;
                recordObj["fileSize"] = QString::number(record.fileSize);
                recordObj["generationTime"] = record.generationTime.toString(Qt::ISODate);
                dataArray.append(recordObj);
            }
            responseObj["data"] = dataArray;
        }

        QJsonDocument doc(responseObj);
        QByteArray jsonBytes = doc.toJson(QJsonDocument::Compact);

        // 构造 HTTP 响应头并发送回前端
        QByteArray response;
        response.append("HTTP/1.1 200 OK\r\n");
        response.append("Content-Type: application/json; charset=utf-8\r\n");
        response.append("Access-Control-Allow-Origin: *\r\n");
        response.append("Connection: close\r\n");
        response.append("Content-Length: " + QByteArray::number(jsonBytes.size()) + "\r\n");
        response.append("\r\n");
        response.append(jsonBytes);

        socket->write(response);
        socket->disconnectFromHost();
    }

    void AudioRecordController::handleDownload(QTcpSocket* socket, const QString& path, const QMap<QString, QString>& params, const QMap<QString, QString>& headers) {
        if (!params.contains("id")) {
            QByteArray response = "HTTP/1.1 400 Bad Request\r\n\r\nMissing 'id' parameter";
            socket->write(response);
            socket->disconnectFromHost();
            return;
        }

        int64_t id = params["id"].toLongLong();
        auto recordRes = m_service->getRecordById(id);
        if (!recordRes.isOk()) {
            QByteArray response = "HTTP/1.1 404 Not Found\r\n\r\nRecord not found in database";
            socket->write(response);
            socket->disconnectFromHost();
            return;
        }

        // 在本地打开音频文件
        QString filePath = recordRes.value().filePath;
        auto* file = new QFile(filePath);
        if (!file->open(QIODevice::ReadOnly)) {
            QByteArray response = "HTTP/1.1 404 Not Found\r\n\r\nFile not found on disk";
            socket->write(response);
            socket->disconnectFromHost();
            delete file;
            return;
        }
        file->setParent(socket);
        // 读取文件信息
        QFileInfo fileInfo(filePath);
        QString fileName = fileInfo.fileName();
        qint64 fileSize = fileInfo.size();

        QByteArray response;
        response.append("HTTP/1.1 200 OK\r\n");
        response.append("Access-Control-Allow-Origin: *\r\n");
        response.append("Content-Type: audio/wav\r\n");
        response.append("Content-Disposition: attachment; filename=\"" + fileName.toUtf8() + "\"\r\n");
        response.append("Content-Length: " + QByteArray::number(fileSize) + "\r\n");
        response.append("Connection: close\r\n");
        response.append("\r\n");
        socket->write(response);

        QObject::connect(socket, &QTcpSocket::bytesWritten, file, [socket, file](qint64 bytes) {
            if (!file->isOpen()) return;
            if (!file->atEnd()) {
                QByteArray chunk = file->read(64 * 1024);
                socket->write(chunk);
            } else {
                file->close();
                socket->disconnectFromHost();
                file->deleteLater();
            }
        });

        // 发送文件
        if (!file->atEnd()) {
            QByteArray chunk = file->read(64 * 1024);
            socket->write(chunk);
        } else {
            file->close();
            socket->disconnectFromHost();
            file->deleteLater();
        }
    }
}