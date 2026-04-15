#include "AudioRecordController.h"
#include "../NetworkResponse.h"
#include "utils/JwtUtils.h"
#include "../../core/Config.h"

namespace radar::network {
    AudioRecordController::AudioRecordController(QObject* parent) : QObject(parent) {
        m_service = std::make_unique<AudioRecordService>();
        m_httpServer = std::make_unique<QHttpServer>();
    }

    AudioRecordController::~AudioRecordController() {
        stop();
    }

    Result<void> AudioRecordController::init(const DatabaseConfig& dbConfig, const NetworkConfig& netConfig) {
        m_port = netConfig.port;
        m_bindAddress = QHostAddress(netConfig.bindAddress);
        if (network::serverSecret.isEmpty()) {
            return Result<void>::error("Server secret is missing in configuration", ErrorCode::InvalidConfig);
        }
        m_jwtSecret = netConfig.serverSecret;
        auto res = m_service->init(dbConfig, netConfig);
        if (!res.isOk()) {
            return Result<void>::error("Service init failed: " + res.errorMessage(), res.errorCode());
        }
        setupRoutes();
        return Result<void>::ok();
    }

    Result<void> AudioRecordController::start() const {
        if (!findChildren<QTcpServer*>().isEmpty()) {
            return Result<void>::error("Server already started", ErrorCode::InvalidState);
        }
        m_service->start();
        // 绑定 IP 和端口
        auto* tcpServer = new QTcpServer(const_cast<AudioRecordController*>(this));
        if (!tcpServer->listen(m_bindAddress, static_cast<quint16>(m_port))) {
            delete tcpServer;
            return Result<void>::error("HttpServer listen failed", ErrorCode::NetworkListenFailed);
        }
        m_httpServer->bind(tcpServer);
        return Result<void>::ok();
    }

    void AudioRecordController::stop() {
        if (QThreadPool::globalInstance()) {
            QThreadPool::globalInstance()->waitForDone();
        }
    }

    void AudioRecordController::setupRoutes() const {
        auto optionsHandler = [](const QHttpServerRequest&) { return NetworkResponse::success(); };
        m_httpServer->route("/api/files", QHttpServerRequest::Method::Options, optionsHandler);
        m_httpServer->route("/api/download", QHttpServerRequest::Method::Options, optionsHandler);
        m_httpServer->route("/api/files", QHttpServerRequest::Method::Get,
            [this](const QHttpServerRequest& req) { return handleListFiles(req); });
        m_httpServer->route("/api/download", QHttpServerRequest::Method::Get,
            [this](const QHttpServerRequest& req, QHttpServerResponder& responder) {
                handleDownload(req, responder);
            });
    }

    Result<qint64> AudioRecordController::checkAuth(const QHttpServerRequest& request) const {
        QString authHeader = QString::fromUtf8(request.value("Authorization")).trimmed();
        if (authHeader.isEmpty() || !authHeader.startsWith("Bearer ", Qt::CaseInsensitive)) {
            return Result<qint64>::error("Missing or invalid Authorization header scheme", ErrorCode::AuthorizationFailed);
        }
        QString token = authHeader.mid(7).trimmed();

        return JwtUtils::verifyToken(token, m_jwtSecret);
    }

    QHttpServerResponse AudioRecordController::handleListFiles(const QHttpServerRequest& request) const {
        auto authRes = checkAuth(request);
        if (!authRes.isOk()) {
            return NetworkResponse::error(static_cast<int>(ErrorCode::AuthorizationFailed), "Unauthorized", QHttpServerResponse::StatusCode::Unauthorized);
        }
        //TODO: 处理 UID
        qint64 uid = authRes.value();
        // 提取 URL 参数
        QUrlQuery query = request.query();
        int limit = query.hasQueryItem("limit") ? query.queryItemValue("limit").toInt() : 20;
        int offset = query.hasQueryItem("offset") ? query.queryItemValue("offset").toInt() : 0;
        if (limit <= 0) limit = 20;

        QDateTime startTime;
        QDateTime endTime;
        if (query.hasQueryItem("startTime")) startTime = QDateTime::fromString(query.queryItemValue("startTime"), Qt::ISODate);
        if (query.hasQueryItem("endTime")) endTime = QDateTime::fromString(query.queryItemValue("endTime"), Qt::ISODate);

        auto recordsRes = m_service->getRecordPage(startTime, endTime, limit, offset);
        if (!recordsRes.isOk()) {
            return NetworkResponse::error(static_cast<int>(recordsRes.errorCode()), recordsRes.errorMessage(), QHttpServerResponse::StatusCode::InternalServerError);
        }

        auto countRes = m_service->getTotalCount(startTime, endTime);
        if (!countRes.isOk()) {
            return NetworkResponse::error(static_cast<int>(countRes.errorCode()), countRes.errorMessage(), QHttpServerResponse::StatusCode::InternalServerError);
        }

        PageDTO<AudioRecordDTO> pageData;
        pageData.list = recordsRes.value();
        pageData.total = countRes.value();
        return NetworkResponse::success(pageData);
    }

   void AudioRecordController::handleDownload(const QHttpServerRequest& request, QHttpServerResponder& responder) const {
        auto authRes = checkAuth(request);
        if (!authRes.isOk()) {
            NetworkResponse::writeError(responder, static_cast<int>(ErrorCode::AuthorizationFailed), "Unauthorized", QHttpServerResponder::StatusCode::Unauthorized);
            return;
        }
        QUrlQuery query = request.query();
        if (!query.hasQueryItem("id")) {
            NetworkResponse::writeError(responder, static_cast<int>(ErrorCode::InvalidParam), "Missing 'id' parameter", QHttpServerResponder::StatusCode::Unauthorized);
            return;
        }

        qint64 id = query.queryItemValue("id").toLongLong();
        qint64 speed = query.hasQueryItem("speed") ? query.queryItemValue("speed").toLongLong() * 1024 : 0;
        QString rangeHeader = QString::fromUtf8(request.value("Range"));

        auto downloadRes = m_service->prepareDownload(id, speed, rangeHeader, const_cast<AudioRecordController*>(this));
        if (!downloadRes.isOk()) {
            NetworkResponse::writeError(responder, static_cast<int>(downloadRes.errorCode()), downloadRes.errorMessage(), QHttpServerResponder::StatusCode::NotFound);            return;
        }

        const auto& context = downloadRes.value();
        connect(context.stream, &QIODevice::aboutToClose, context.stream, &QObject::deleteLater);
        
        QHttpHeaders headers = NetworkResponse::getCorsHeaders();
        headers.append("Content-Disposition", QString("attachment; filename=\"%1\"").arg(context.fileName).toUtf8());
        headers.append("Accept-Ranges", "bytes");
        headers.append("Content-Type", context.contentType.toUtf8());

        QHttpServerResponder::StatusCode statusCode = QHttpServerResponder::StatusCode::Ok;

        if (context.isPartial) {
            headers.append("Content-Range", QString("bytes %1-%2/%3").arg(context.startPos).arg(context.endPos).arg(context.fileSize).toUtf8());
            statusCode = QHttpServerResponder::StatusCode::PartialContent;
        }
        responder.write(context.stream, headers, statusCode);
    }
}