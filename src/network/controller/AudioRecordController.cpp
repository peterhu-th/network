#include "AudioRecordController.h"
#include "../NetworkResponse.h"
#include "../utils/AuthMiddleware.h"
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
        if (netConfig.serverSecret.isEmpty()) {
            return Result<void>::error("Server secret is missing in configuration", ErrorCode::InvalidConfig);
        }
        m_jwtSecret = netConfig.serverSecret;
        auto res = m_service->init(dbConfig, netConfig);
        if (!res.isOk()) {
            return Result<void>::error("Service init failed: " + res.errorMessage(), res.errorCode());
        }

        // 初始化鉴权模块依赖
        m_userMapper = std::make_shared<UserMapper>(netConfig.globalConnectionName);
        auto smtpConfig = Config::instance().smtpConfig();
        m_smtpClient = std::make_shared<SmtpClient>(
                    smtpConfig.server.toStdString(),
                    smtpConfig.port,
                    smtpConfig.username.toStdString(),
                    smtpConfig.password.toStdString()
        );
        m_authService = std::make_shared<AuthService>(m_userMapper, m_smtpClient, m_jwtSecret);
        m_adminService = std::make_shared<AdminService>(m_userMapper);

        m_authController = std::make_unique<AuthController>(m_authService);
        m_adminController = std::make_unique<AdminController>(m_adminService, m_authService);

        setupRoutes();
        m_authController->setupRoutes(*m_httpServer, m_jwtSecret);
        m_adminController->setupRoutes(*m_httpServer, m_jwtSecret, m_userMapper);

        return Result<void>::ok();
    }

    Result<void> AudioRecordController::start() {
        if (!findChildren<QTcpServer*>().isEmpty()) {
            return Result<void>::error("Server already started", ErrorCode::InvalidState);
        }
        m_service->start();
        // 绑定 IP 和端口
        auto* tcpServer = new QTcpServer(this);
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

    void AudioRecordController::setupRoutes() {
        auto optionsHandler = [](const QHttpServerRequest&) { return NetworkResponse::success(); };
        m_httpServer->route("/files", QHttpServerRequest::Method::Options, optionsHandler);
        m_httpServer->route("/download", QHttpServerRequest::Method::Options, optionsHandler);
        m_httpServer->route("/download/batch", QHttpServerRequest::Method::Options, optionsHandler);
        m_httpServer->route("/download/batch/file/<arg>", QHttpServerRequest::Method::Options, [](const QString&, const QHttpServerRequest&) { return NetworkResponse::success(); });

        m_httpServer->route("/files", QHttpServerRequest::Method::Get,
            AuthMiddleware::wrap(AuthLevel::Guest, m_jwtSecret, [this](const QHttpServerRequest& req) {
                qDebug().noquote() << "[Network Request] GET /files from" << req.remoteAddress().toString();
                return handleListFiles(req);
            }, m_userMapper));

        m_httpServer->route("/download", QHttpServerRequest::Method::Get,
            AuthMiddleware::wrapAsync(AuthLevel::Guest, m_jwtSecret, [this](const QHttpServerRequest& req, QHttpServerResponder& responder) {
                qDebug().noquote() << "[Network Request] GET /download from" << req.remoteAddress().toString();
                handleDownload(req, responder);
            }, m_userMapper));

        m_httpServer->route("/download/batch", QHttpServerRequest::Method::Post,
            AuthMiddleware::wrap(AuthLevel::Guest, m_jwtSecret, [this](const QHttpServerRequest& req) {
                qDebug().noquote() << "[Network Request] POST /download/batch from" << req.remoteAddress().toString();
                return handleBatchDownloadJob(req);
            }, m_userMapper));

        m_httpServer->route("/download/batch/file/<arg>", QHttpServerRequest::Method::Get,
            AuthMiddleware::wrapAsync1(AuthLevel::Guest, m_jwtSecret, [this](const QString& taskId, const QHttpServerRequest& req, QHttpServerResponder& responder) {
                qDebug().noquote() << "[Network Request] GET /download/batch/file/" << taskId << "from" << req.remoteAddress().toString();
                handleBatchDownloadFile(req, responder, taskId);
            }, m_userMapper));

        m_httpServer->route("/files/<arg>", QHttpServerRequest::Method::Delete,
            AuthMiddleware::wrap1(AuthLevel::Admin, m_jwtSecret, [this](const QString& id, const QHttpServerRequest& req) {
                qDebug().noquote() << "[Network Request] DELETE /files/" << id << "from" << req.remoteAddress().toString();
                return handleDeleteFile(id, req);
            }, m_userMapper));
    }

    QHttpServerResponse AudioRecordController::handleListFiles(const QHttpServerRequest& request) const {
        // 提取 URL 参数
        QUrlQuery query = request.query();
        int limit = query.hasQueryItem("limit") ? query.queryItemValue("limit").toInt() : 20;
        int offset = query.hasQueryItem("offset") ? query.queryItemValue("offset").toInt() : 0;
        if (limit <= 0) limit = 20;
        // 接收刷新请求时重新扫描本地文件并更新数据库
        if (query.hasQueryItem("forceScan") && query.queryItemValue("forceScan") == "true") {
            qDebug() << "Executing manual scan on request...";
            auto res = m_service->forceScan();
            if (!res.isOk()) {
                return NetworkResponse::error(static_cast<int>(res.errorCode()), res.errorMessage(), QHttpServerResponse::StatusCode::InternalServerError);
            }
        }
        QDateTime startTime;
        QDateTime endTime;
        if (query.hasQueryItem("startTime")) startTime = QDateTime::fromString(query.queryItemValue("startTime"), Qt::ISODate);
        if (query.hasQueryItem("endTime")) endTime = QDateTime::fromString(query.queryItemValue("endTime"), Qt::ISODate);
        QString format = query.hasQueryItem("format") ? query.queryItemValue("format") : "";

        auto recordsRes = m_service->getRecordPage(startTime, endTime, format, limit, offset);
        if (!recordsRes.isOk()) {
            return NetworkResponse::error(static_cast<int>(recordsRes.errorCode()), recordsRes.errorMessage(), QHttpServerResponse::StatusCode::InternalServerError);
        }
        auto countRes = m_service->getTotalCount(startTime, endTime, format);
        if (!countRes.isOk()) {
            return NetworkResponse::error(static_cast<int>(countRes.errorCode()), countRes.errorMessage(), QHttpServerResponse::StatusCode::InternalServerError);
        }

        PageDTO<AudioRecordDTO> pageData;
        pageData.list = recordsRes.value();
        pageData.total = countRes.value();
        return NetworkResponse::fromResult(Result<PageDTO<AudioRecordDTO>>::ok(pageData));
    }

    QHttpServerResponse AudioRecordController::handleDeleteFile(const QString& idStr, const QHttpServerRequest& request) const {
        bool ok;
        qint64 id = idStr.toLongLong(&ok);
        if (!ok) {
            return NetworkResponse::error(static_cast<int>(ErrorCode::InvalidParam), "Invalid ID format", QHttpServerResponse::StatusCode::BadRequest);
        }
        auto res = m_service->deleteRecord(id);
        if (!res.isOk()) {
            return NetworkResponse::error(static_cast<int>(res.errorCode()), res.errorMessage(), QHttpServerResponse::StatusCode::InternalServerError);
        }
        return NetworkResponse::success();
    }

   void AudioRecordController::handleDownload(const QHttpServerRequest& request, QHttpServerResponder& responder) {
        QUrlQuery query = request.query();
        if (!query.hasQueryItem("id")) {
            NetworkResponse::writeError(responder, static_cast<int>(ErrorCode::InvalidParam), "Missing 'id' parameter", QHttpServerResponder::StatusCode::Unauthorized);
            return;
        }
        // 设置限速
        qint64 id = query.queryItemValue("id").toLongLong();
        qint64 speed = query.hasQueryItem("speed") ? query.queryItemValue("speed").toLongLong() * 1024 : 0;
        QString rangeHeader = QString::fromUtf8(request.value("Range"));

        auto downloadRes = m_service->prepareDownload(id, speed, rangeHeader, this);
        if (!downloadRes.isOk()) {
            NetworkResponse::writeError(responder, static_cast<int>(downloadRes.errorCode()), downloadRes.errorMessage(), QHttpServerResponder::StatusCode::NotFound);
            return;
        }
        const auto& context = downloadRes.value();
        connect(context.stream, &QIODevice::aboutToClose, context.stream, &QObject::deleteLater);
        // 解析跨域头
        QHttpHeaders headers = NetworkResponse::getCorsHeaders();
        headers.append("Content-Disposition", QString("attachment; filename=\"%1\"").arg(context.fileName).toUtf8());
        headers.append("Accept-Ranges", "bytes");
        headers.append("Content-Type", context.contentType.toUtf8());
        QHttpServerResponder::StatusCode statusCode = QHttpServerResponder::StatusCode::Ok;
        // 断点续传
        if (context.isPartial) {
            headers.append("Content-Range", QString("bytes %1-%2/%3").arg(context.startPos).arg(context.endPos).arg(context.fileSize).toUtf8());
            statusCode = QHttpServerResponder::StatusCode::PartialContent;
        }
        responder.write(context.stream, headers, statusCode);
    }

    QHttpServerResponse AudioRecordController::handleBatchDownloadJob(const QHttpServerRequest& request) const {
        QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if (doc.isNull() || !doc.object().contains("ids")) {
            return NetworkResponse::error(static_cast<int>(ErrorCode::InvalidParam), "Missing ids array", QHttpServerResponse::StatusCode::BadRequest);
        }
        QJsonArray idsArray = doc.object()["ids"].toArray();
        QList<qint64> ids;
        for (const QJsonValue v : idsArray) {
            ids.append(v.toVariant().toLongLong());
        }
        auto res = m_service->getOrSubmitBatchJob(ids);
        if (!res.isOk()) {
            return NetworkResponse::error(static_cast<int>(res.errorCode()), res.errorMessage(), QHttpServerResponse::StatusCode::InternalServerError);
        }
        return NetworkResponse::success(res.value().toJson());
    }

    void AudioRecordController::handleBatchDownloadFile(const QHttpServerRequest& request, QHttpServerResponder& responder, const QString& taskId) {
        // 获取文件流
        auto res = m_service->getBatchFile(taskId, this);
        if (!res.isOk()) {
            NetworkResponse::writeError(responder, static_cast<int>(res.errorCode()), res.errorMessage(), QHttpServerResponder::StatusCode::NotFound);
            return;
        }
        const auto& context = res.value();
        // 文件流关闭时释放内存
        connect(context.stream, &QIODevice::aboutToClose, context.stream, &QObject::deleteLater);
        QHttpHeaders headers = NetworkResponse::getCorsHeaders();
        QByteArray encodedFileName = QUrl::toPercentEncoding(context.fileName);
        headers.append("Content-Disposition", QByteArray("attachment; filename*=UTF-8''") + encodedFileName);
        headers.append("Content-Type", context.contentType.toUtf8());
        responder.write(context.stream, headers, QHttpServerResponder::StatusCode::Ok);
    }
}
