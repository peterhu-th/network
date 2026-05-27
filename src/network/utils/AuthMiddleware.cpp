#include "AuthMiddleware.h"
#include "../NetworkResponse.h"

namespace radar::network {

    Result<TokenPayload> AuthMiddleware::checkAuth(const QHttpServerRequest& request, const QString& jwtSecret,
                                                   const std::shared_ptr<UserMapper>& userMapper) {
        QString authHeader = QString::fromUtf8(request.value("Authorization")).trimmed();
        if (authHeader.isEmpty() || !authHeader.startsWith("Bearer ", Qt::CaseInsensitive)) {
            return Result<TokenPayload>::error("Missing or invalid Authorization header scheme", ErrorCode::AuthorizationFailed);
        }
        QString token = authHeader.mid(7).trimmed();
        auto tokenRes = JwtUtils::verifyToken(token, jwtSecret);
        if (!tokenRes.isOk() || !userMapper) {
            return tokenRes;
        }

        auto userRes = userMapper->findById(tokenRes.value().uid);
        if (!userRes.isOk()) {
            return Result<TokenPayload>::error("Failed to load authenticated user: " + userRes.errorMessage(), userRes.errorCode());
        }
        if (!userRes.value().has_value()) {
            return Result<TokenPayload>::error("Authenticated user no longer exists", ErrorCode::AuthorizationFailed);
        }

        const auto& user = userRes.value().value();
        if (user.status == 0) {
            return Result<TokenPayload>::error("Authenticated user has been disabled", ErrorCode::AuthorizationFailed);
        }

        TokenPayload payload = tokenRes.value();
        QString currentSig = user.passwordHash.length() > 8 ? user.passwordHash.left(8) : user.passwordHash;
        if (!payload.pwdSig.isEmpty() && payload.pwdSig != currentSig) {
            return Result<TokenPayload>::error("Token invalid because user password has changed", ErrorCode::AuthorizationFailed);
        }

        payload.role = user.role;
        return Result<TokenPayload>::ok(payload);
    }

    bool AuthMiddleware::isAuthorized(const TokenPayload& payload, AuthLevel level) {
        if (level == AuthLevel::Public) return true;
        if (level == AuthLevel::Guest) return true;
        if (level == AuthLevel::Admin) return payload.role == 1;
        return false;
    }

    AuthMiddleware::NormalHandler AuthMiddleware::wrap(AuthLevel level, const QString& jwtSecret, const NormalHandler& handler,
                                                       const std::shared_ptr<UserMapper>& userMapper) {
        return [level, jwtSecret, handler, userMapper](const QHttpServerRequest& req) -> QHttpServerResponse {
            if (level != AuthLevel::Public) {
                auto authRes = checkAuth(req, jwtSecret, userMapper);
                if (!authRes.isOk()) {
                    return NetworkResponse::error(static_cast<int>(ErrorCode::AuthorizationFailed), "Unauthorized: " + authRes.errorMessage(), QHttpServerResponse::StatusCode::Unauthorized);
                }
                if (!isAuthorized(authRes.value(), level)) {
                    return NetworkResponse::error(static_cast<int>(ErrorCode::AuthorizationFailed), "Forbidden: insufficient permissions", QHttpServerResponse::StatusCode::Forbidden);
                }
            }
            return handler(req);
        };
    }

    AuthMiddleware::AsyncHandler AuthMiddleware::wrapAsync(AuthLevel level, const QString& jwtSecret, const AsyncHandler& handler,
                                                           const std::shared_ptr<UserMapper>& userMapper) {
        return [level, jwtSecret, handler, userMapper](const QHttpServerRequest& req, QHttpServerResponder& responder) {
            if (level != AuthLevel::Public) {
                auto authRes = checkAuth(req, jwtSecret, userMapper);
                if (!authRes.isOk()) {
                    NetworkResponse::writeError(responder, static_cast<int>(ErrorCode::AuthorizationFailed), "Unauthorized: " + authRes.errorMessage(), QHttpServerResponder::StatusCode::Unauthorized);
                    return;
                }
                if (!isAuthorized(authRes.value(), level)) {
                    NetworkResponse::writeError(responder, static_cast<int>(ErrorCode::AuthorizationFailed), "Forbidden: insufficient permissions", QHttpServerResponder::StatusCode::Forbidden);
                    return;
                }
            }
            handler(req, responder);
        };
    }

}
