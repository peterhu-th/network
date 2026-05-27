#include "AdminController.h"
#include "../NetworkResponse.h"
#include "../utils/AuthMiddleware.h"

namespace radar::network {

    AdminController::AdminController(std::shared_ptr<AdminService> adminService, std::shared_ptr<AuthService> authService, QObject* parent)
        : QObject(parent), m_adminService(std::move(adminService)), m_authService(std::move(authService)) {}

    void AdminController::setupRoutes(QHttpServer& httpServer, const QString& jwtSecret,
                                      const std::shared_ptr<UserMapper>& userMapper) const {
        httpServer.route("/admin/users", QHttpServerRequest::Method::Post,
            AuthMiddleware::wrap(AuthLevel::Admin, jwtSecret, [this](const QHttpServerRequest& req) {
                return handleCreateAdmin(req);
            }, userMapper));

        httpServer.route("/admin/guests", QHttpServerRequest::Method::Get,
            AuthMiddleware::wrap(AuthLevel::Admin, jwtSecret, [this](const QHttpServerRequest& req) {
                return handleGetGuests(req);
            }, userMapper));

        httpServer.route("/admin/guests/<arg>/revoke", QHttpServerRequest::Method::Put,
            AuthMiddleware::wrap1(AuthLevel::Admin, jwtSecret, [this](const QString& id, const QHttpServerRequest& req) {
                return handleRevokeGuest(req, id);
            }, userMapper));
    }

    QHttpServerResponse AdminController::handleCreateAdmin(const QHttpServerRequest& request) const {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(request.body(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            return NetworkResponse::error(static_cast<int>(ErrorCode::InvalidParam), "Invalid JSON format", QHttpServerResponse::StatusCode::BadRequest);
        }

        QJsonObject obj = doc.object();
        QString email = obj.value("email").toString().trimmed();
        QString password = obj.value("password").toString();

        if (email.isEmpty() || password.isEmpty()) {
            return NetworkResponse::error(static_cast<int>(ErrorCode::InvalidParam), "Email and password are required", QHttpServerResponse::StatusCode::BadRequest);
        }

        auto res = m_authService->createAdmin(email, password);
        if (!res.isOk()) {
            return NetworkResponse::error(static_cast<int>(res.errorCode()), res.errorMessage(), QHttpServerResponse::StatusCode::BadRequest);
        }

        return NetworkResponse::success();
    }

    QHttpServerResponse AdminController::handleGetGuests(const QHttpServerRequest& request) const {
        // 提取 URL 参数 limit, offset
        QUrlQuery query = request.query();
        int limit = query.hasQueryItem("limit") ? query.queryItemValue("limit").toInt() : 100;
        int offset = query.hasQueryItem("offset") ? query.queryItemValue("offset").toInt() : 0;
        if (limit <= 0) limit = 100;

        auto res = m_adminService->getGuestList(limit, offset);
        if (!res.isOk()) {
            return NetworkResponse::error(static_cast<int>(res.errorCode()), res.errorMessage(), QHttpServerResponse::StatusCode::InternalServerError);
        }

        QJsonArray arr;
        for (const auto& dto : res.value()) {
            arr.append(dto.toJson());
        }

        return NetworkResponse::success(arr);
    }

    QHttpServerResponse AdminController::handleRevokeGuest(const QHttpServerRequest& request, const QString& userId) const {
        auto res = m_adminService->revokeGuest(userId);
        if (!res.isOk()) {
            return NetworkResponse::error(static_cast<int>(res.errorCode()), res.errorMessage(), QHttpServerResponse::StatusCode::InternalServerError);
        }
        return NetworkResponse::success();
    }

}
