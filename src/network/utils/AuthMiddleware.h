#ifndef AUTH_MIDDLEWARE_H
#define AUTH_MIDDLEWARE_H

#include <QHttpServerRequest>
#include "JwtUtils.h"
#include "../NetworkResponse.h"
#include "../mapper/UserMapper.h"

namespace radar::network {
    enum class AuthLevel {
        Public,
        Guest,
        Admin
    };

    class AuthMiddleware {
    public:
        using NormalHandler = std::function<QHttpServerResponse(const QHttpServerRequest&)>;
        using AsyncHandler = std::function<void(const QHttpServerRequest&, QHttpServerResponder&)>;

        static NormalHandler wrap(AuthLevel level, const QString& jwtSecret, const NormalHandler& handler,
                                  const std::shared_ptr<UserMapper>& userMapper = nullptr);

        static AsyncHandler wrapAsync(AuthLevel level, const QString& jwtSecret, const AsyncHandler& handler,
                                      const std::shared_ptr<UserMapper>& userMapper = nullptr);

        template<typename Func>
        static auto wrap1(AuthLevel level, const QString& jwtSecret, Func handler,
                          const std::shared_ptr<UserMapper>& userMapper = nullptr) {
            return [level, jwtSecret, handler, userMapper](const QString& arg1, const QHttpServerRequest& req) -> QHttpServerResponse {
                if (level != AuthLevel::Public) {
                    auto authRes = checkAuth(req, jwtSecret, userMapper);
                    if (!authRes.isOk()) {
                        return NetworkResponse::error(static_cast<int>(ErrorCode::AuthorizationFailed), "Unauthorized: " + authRes.errorMessage(), QHttpServerResponse::StatusCode::Unauthorized);
                    }
                    if (!isAuthorized(authRes.value(), level)) {
                        return NetworkResponse::error(static_cast<int>(ErrorCode::AuthorizationFailed), "Forbidden: insufficient permissions", QHttpServerResponse::StatusCode::Forbidden);
                    }
                }
                return handler(arg1, req);
            };
        }

        template<typename Func>
        static auto wrapAsync1(AuthLevel level, const QString& jwtSecret, Func handler,
                               const std::shared_ptr<UserMapper>& userMapper = nullptr) {
            return [level, jwtSecret, handler, userMapper](const QString& arg1, const QHttpServerRequest& req, QHttpServerResponder& responder) {
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
                handler(arg1, req, responder);
            };
        }

        static Result<TokenPayload> checkAuth(const QHttpServerRequest& request, const QString& jwtSecret,
                                              const std::shared_ptr<UserMapper>& userMapper = nullptr);
        static bool isAuthorized(const TokenPayload& payload, AuthLevel level);
    };
}

#endif
