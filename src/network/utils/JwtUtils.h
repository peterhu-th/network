#ifndef JWT_UTILS_H
#define JWT_UTILS_H

#include <jwt-cpp/jwt.h>
#include "../../core/Types.h"

namespace radar::network {
    struct TokenPayload {
        QString uid;
        int role = 0;
        QString pwdSig;
    };

    class JwtUtils {
    public:
        static QString generateToken(const QString& uid, int role, const QString& passwordHash, const QString& secret) {
            QString sig = passwordHash.length() > 8 ? passwordHash.left(8) : passwordHash;
            auto token = jwt::create()
                .set_type("JWT")
                .set_payload_claim("uid", jwt::claim(uid.toStdString()))
                .set_payload_claim("role", jwt::claim(std::to_string(role)))
                .set_payload_claim("pwd_sig", jwt::claim(sig.toStdString()))
                // 30 分钟内有效
                .set_expires_at(std::chrono::system_clock::now() + std::chrono::minutes{30})
                .sign(jwt::algorithm::hs256{secret.toStdString()});
            return QString::fromStdString(token);
        }

        static Result<TokenPayload> verifyToken(const QString& tokenStr, const QString& secret) {
            try {
                auto decoded = jwt::decode(tokenStr.toStdString());
                auto verifier = jwt::verify()
                    .allow_algorithm(jwt::algorithm::hs256{secret.toStdString()});
                verifier.verify(decoded);
                auto uidStr = decoded.get_payload_claim("uid").as_string();
                int role = 0;
                if (decoded.has_payload_claim("role")) {
                    role = std::stoi(decoded.get_payload_claim("role").as_string());
                }
                QString pwdSig;
                if (decoded.has_payload_claim("pwd_sig")) {
                    pwdSig = QString::fromStdString(decoded.get_payload_claim("pwd_sig").as_string());
                }
                TokenPayload payload;
                payload.uid = QString::fromStdString(uidStr);
                payload.role = role;
                payload.pwdSig = pwdSig;
                return Result<TokenPayload>::ok(payload);

            } catch (const jwt::error::token_verification_exception& e) {
                return Result<TokenPayload>::error(QString("Token verify failed: ") + e.what(), ErrorCode::AuthorizationFailed);
            } catch (const std::exception& e) {
                return Result<TokenPayload>::error(QString("Invalid token format: ") + e.what(), ErrorCode::AuthorizationFailed);
            }
        }
    };
}
#endif