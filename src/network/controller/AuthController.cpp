#include "AuthController.h"
#include "../NetworkResponse.h"
#include "../utils/AuthMiddleware.h"
#include <QJsonDocument>

namespace radar::network {


    namespace {
        // 提取公共的 JSON 解析逻辑，遵循 DRY 原则
        std::optional<QJsonObject> parseJsonBody(const QHttpServerRequest& request, QHttpServerResponse& errorResponse) {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(request.body(), &parseError);
            if (parseError.error != QJsonParseError::NoError) {
                errorResponse = NetworkResponse::error(static_cast<int>(ErrorCode::InvalidParam), "Invalid JSON format", QHttpServerResponse::StatusCode::BadRequest);
                return std::nullopt;
            }
            return doc.object();
        }
    }

    AuthController::AuthController(std::shared_ptr<AuthService> authService, QObject* parent)
        : QObject(parent), m_authService(std::move(authService)) {}

    void AuthController::setupRoutes(QHttpServer& httpServer, const QString& jwtSecret) const {
        auto optionsHandler = [](const QHttpServerRequest&) { return NetworkResponse::success(); };
        httpServer.route("/auth/send-code", QHttpServerRequest::Method::Options, optionsHandler);
        httpServer.route("/auth/register", QHttpServerRequest::Method::Options, optionsHandler);
        httpServer.route("/auth/login", QHttpServerRequest::Method::Options, optionsHandler);

        httpServer.route("/auth/send-code", QHttpServerRequest::Method::Post,
            AuthMiddleware::wrap(AuthLevel::Public, jwtSecret, [this](const QHttpServerRequest& req) {
                return handleSendCode(req);
            }));

        httpServer.route("/auth/register", QHttpServerRequest::Method::Post,
            AuthMiddleware::wrap(AuthLevel::Public, jwtSecret, [this](const QHttpServerRequest& req) {
                return handleRegister(req);
            }));

        httpServer.route("/auth/login", QHttpServerRequest::Method::Post,
            AuthMiddleware::wrap(AuthLevel::Public, jwtSecret, [this](const QHttpServerRequest& req) {
                return handleLogin(req);
            }));
    }

    QHttpServerResponse AuthController::handleSendCode(const QHttpServerRequest& request) const {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(request.body(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            return NetworkResponse::error(static_cast<int>(ErrorCode::InvalidParam), "Invalid JSON format", QHttpServerResponse::StatusCode::BadRequest);
        }
        QString email = doc.object().value("email").toString().trimmed();
        if (email.isEmpty()) {
            return NetworkResponse::error(static_cast<int>(ErrorCode::InvalidParam), "Email is required", QHttpServerResponse::StatusCode::BadRequest);
        }
        auto res = m_authService->sendVerificationCode(email);
        if (!res.isOk()) {
            return NetworkResponse::error(static_cast<int>(res.errorCode()), res.errorMessage(), QHttpServerResponse::StatusCode::InternalServerError);
        }
        return NetworkResponse::success();
    }

    QHttpServerResponse AuthController::handleRegister(const QHttpServerRequest& request) const {
        QHttpServerResponse errResp = NetworkResponse::success();
        auto jsonOpt = parseJsonBody(request, errResp);
        if (!jsonOpt) return errResp;
        QString email = jsonOpt->value("email").toString().trimmed();
        QString code = jsonOpt->value("code").toString().trimmed();
        QString password = jsonOpt->value("password").toString();

        if (email.isEmpty() || code.isEmpty() || password.isEmpty()) {
            return NetworkResponse::error(static_cast<int>(ErrorCode::InvalidParam), "Email, code, and password are required", QHttpServerResponse::StatusCode::BadRequest);
        }
        auto res = m_authService->registerUser(email, code, password);
        if (!res.isOk()) {
            return NetworkResponse::error(static_cast<int>(res.errorCode()), res.errorMessage(), QHttpServerResponse::StatusCode::BadRequest);
        }
        return NetworkResponse::success();
    }

    QHttpServerResponse AuthController::handleLogin(const QHttpServerRequest& request) const {
        QHttpServerResponse errResp = NetworkResponse::success();
        auto jsonOpt = parseJsonBody(request, errResp);
        if (!jsonOpt) return errResp;
        QString email = jsonOpt->value("email").toString().trimmed();
        QString password = jsonOpt->value("password").toString();

        if (email.isEmpty() || password.isEmpty()) {
            return NetworkResponse::error(static_cast<int>(ErrorCode::InvalidParam), "Email and password are required", QHttpServerResponse::StatusCode::BadRequest);
        }
        auto res = m_authService->login(email, password);
        if (!res.isOk()) {
            return NetworkResponse::error(static_cast<int>(res.errorCode()), res.errorMessage(), QHttpServerResponse::StatusCode::Unauthorized);
        }
        return NetworkResponse::success(res.value());
    }
}
