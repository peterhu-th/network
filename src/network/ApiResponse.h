#ifndef API_RESPONSE_H
#define API_RESPONSE_H

#include <QHttpServerResponse>
#include <QHttpHeaders>
#include <QJsonObject>
#include <QJsonArray>
#include <QTcpServer>

namespace radar::network {
    class ApiResponse {
    public:
        // 成功响应 (包含数据)
        static QHttpServerResponse success(const QJsonValue& data) {
            QJsonObject res{
                        {"code", 20000},
                        {"message", "success"},
                        {"data", data}
            };
            QHttpServerResponse response(res, QHttpServerResponse::StatusCode::Ok);
            appendCorsHeaders(response);
            return response;
        }

        // 成功响应 (无数据)
        static QHttpServerResponse success() {
            return success(QJsonValue::Null);
        }

        // 错误响应 (返回内部 ErrorCode 和 HTTP 状态码)
        static QHttpServerResponse error(int errorCode, const QString& message, QHttpServerResponse::StatusCode httpCode) {
            QJsonObject res{
                        {"code", errorCode},
                        {"message", message},
                        {"data", QJsonValue::Null}
            };
            QHttpServerResponse response(res, httpCode);
            appendCorsHeaders(response);
            return response;
        }

        static void writeError(QHttpServerResponder& responder, int errorCode, const QString& message, QHttpServerResponder::StatusCode httpCode) {
            QJsonObject res{
                        {"code", errorCode},
                        {"message", message},
                        {"data", QJsonValue::Null}
            };
            QHttpHeaders headers;
            headers.append("Access-Control-Allow-Origin", "*");
            headers.append("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            headers.append("Access-Control-Allow-Headers", "Content-Type, Authorization");
            headers.append("Content-Type", "application/json");
            responder.write(QJsonDocument(res).toJson(), headers, httpCode);
        }

    private:
        // 添加跨域头
        static void appendCorsHeaders(QHttpServerResponse& response) {
            QHttpHeaders headers;
            headers.append("Access-Control-Allow-Origin", "*");
            headers.append("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            headers.append("Access-Control-Allow-Headers", "Content-Type, Authorization");
            response.setHeaders(headers);
        }
    };
}
#endif