#ifndef JWT_UTILS_H
#define JWT_UTILS_H

#include <QString>
#include <string>
#include <chrono>
#include <jwt-cpp/jwt.h>
#include "../../core/Types.h"

namespace radar::network {
    class JwtUtils {
    public:
        // 生成 Token (使用标准 JWT type 声明)
        static QString generateToken(qint64 uid, const QString& secret) {
            auto token = jwt::create()
                .set_type("JWT")
                .set_payload_claim("uid", jwt::claim(std::to_string(uid)))
                .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours{24})
                .sign(jwt::algorithm::hs256{secret.toStdString()});
            return QString::fromStdString(token);
        }

        // 验证 Token
        static Result<qint64> verifyToken(const QString& tokenStr, const QString& secret) {
            try {
                auto decoded = jwt::decode(tokenStr.toStdString());
                auto verifier = jwt::verify()
                    .allow_algorithm(jwt::algorithm::hs256{secret.toStdString()});

                verifier.verify(decoded);

                auto uidStr = decoded.get_payload_claim("uid").as_string();
                return Result<qint64>::ok(QString::fromStdString(uidStr).toLongLong());

            } catch (const jwt::error::token_verification_exception& e) {
                return Result<qint64>::error(QString("Token verify failed: ") + e.what(), ErrorCode::AuthorizationFailed);
            } catch (const std::exception& e) {
                return Result<qint64>::error(QString("Invalid token format: ") + e.what(), ErrorCode::AuthorizationFailed);
            }
        }
    };
}
#endif