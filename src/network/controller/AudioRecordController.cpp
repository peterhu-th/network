#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QThreadPool>
#include <QElapsedTimer>
#include <algorithm>
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

        // 在本地打开音频文件或其关联的 JSON 元数据文件
        QString filePath = recordRes.value().filePath;
        QString contentType = "audio/wav";
        if (params.value("type") == "json") {
            if (filePath.endsWith(".wav", Qt::CaseInsensitive)) {
                filePath.replace(filePath.length() - 4, 4, ".json");
            } else {
                filePath += ".json";
            }
            contentType = "application/json; charset=utf-8";
        }
        
        qDebug() << "[Downloader] Attempting to open file at path:" << filePath << " for type=" << params.value("type");

        auto* file = new QFile(filePath);
        if (!file->open(QIODevice::ReadOnly)) {
            qWarning() << "[Downloader] Failed to open file:" << filePath << ", error:" << file->errorString();
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

        qint64 startPos = 0;
        qint64 endPos = fileSize - 1;
        bool isPartialContent = false;

        // 断点续传支持
        if (headers.contains("range")) {
            QString rangeHeader = headers["range"];
            if (rangeHeader.startsWith("bytes=", Qt::CaseInsensitive)) {
                QString rangeStr = rangeHeader.mid(6);
                QStringList parts = rangeStr.split("-");
                if (!parts.isEmpty()) {
                    if (!parts[0].isEmpty()) {
                        startPos = parts[0].toLongLong();
                    }
                    if (parts.size() >= 2 && !parts[1].isEmpty()) {
                        endPos = parts[1].toLongLong();
                    }
                }
                isPartialContent = true;

                // 校验请求范围是否合法
                if (startPos >= fileSize) {
                    QByteArray response = "HTTP/1.1 416 Range Not Satisfiable\r\n";
                    response.append("Content-Range: bytes */" + QByteArray::number(fileSize) + "\r\n\r\n");
                    socket->write(response);
                    socket->disconnectFromHost();
                    delete file;
                    return;
                }
                if (endPos >= fileSize) {
                    endPos = fileSize - 1;
                }
            }
        }

        qint64 contentLength = endPos - startPos + 1;
        file->seek(startPos);
        
        // 动态分配堆内存追踪剩余字节和限速状态
        struct DownloadState {
            qint64 remaining = 0;
            qint64 limitBytesPerSec = 1e6;
            qint64 totalSent = 0;
            QElapsedTimer timer;
            QMetaObject::Connection connection;
        };
        auto state = std::make_shared<DownloadState>();
        state->remaining = contentLength;
        state->totalSent = 0;
        state->limitBytesPerSec = params.value("speed", "0").toLongLong() * 1024;
        state->timer.start();

        QByteArray response;
        if (isPartialContent) {
            response.append("HTTP/1.1 206 Partial Content\r\n");
            response.append("Content-Range: bytes " + QByteArray::number(startPos) + "-" + QByteArray::number(endPos) + "/" + QByteArray::number(fileSize) + "\r\n");
        } else {
            response.append("HTTP/1.1 200 OK\r\n");
        }
        response.append("Access-Control-Allow-Origin: *\r\n");
        response.append("Content-Type: " + contentType.toUtf8() + "\r\n");
        response.append("Content-Disposition: attachment; filename=\"" + fileName.toUtf8() + "\"\r\n");
        response.append("Content-Length: " + QByteArray::number(contentLength) + "\r\n");
        response.append("Accept-Ranges: bytes\r\n");
        response.append("Connection: close\r\n\r\n");
        socket->write(response);

        auto sendNextChunk = std::make_shared<std::function<void()>>();
        *sendNextChunk = [socket, file, state, sendNextChunk]() {
            if (!file->isOpen()) return;

            // 限制 Qt 发送缓冲区大小
            if (socket->bytesToWrite() > 128 * 1024) return;

            if (state->remaining <= 0) {
                if (socket->bytesToWrite() == 0) {
                    file->close();
                    socket->disconnectFromHost();
                    file->deleteLater();
                    QObject::disconnect(state->connection);
                    *sendNextChunk = nullptr;
                }
                return;
            }

            // 限速检查机制
            if (state->limitBytesPerSec > 0 && state->totalSent > 0) {
                qint64 expectedMs = (state->totalSent * 1000) / state->limitBytesPerSec;
                qint64 actualMs = state->timer.elapsed();
                if (actualMs < expectedMs) {
                    QTimer::singleShot(expectedMs - actualMs, socket, *sendNextChunk);
                    return;
                }
            }
            qint64 maxChunk = 64 * 1024;
            if (state->limitBytesPerSec > 0) {
                maxChunk = qMax((qint64)1024, state->limitBytesPerSec / 10);
            }
            qint64 toRead = std::min(maxChunk, state->remaining);

            QByteArray chunk = file->read(toRead);
            state->remaining -= chunk.size();
            state->totalSent += chunk.size();
            socket->write(chunk);
        };
        // 触发下一次写入
        state->connection = QObject::connect(socket, &QTcpSocket::bytesWritten, file, [sendNextChunk](qint64 bytes) {
            if (*sendNextChunk) {
                (*sendNextChunk)();
            }
        });
        (*sendNextChunk)();
    }
}