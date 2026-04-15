#ifndef API_RESPONSE_H
#define API_RESPONSE_H

#include <QHttpServerResponse>
#include <QHttpHeaders>
#include <QJsonObject>
#include <QTcpServer>

namespace radar::network {
    class NetworkResponse {
    public:
        template <typename T>
        static QHttpServerResponse fromResult(const Result<T>& result, QHttpServerResponse::StatusCode errHttpCode = QHttpServerResponse::StatusCode::BadRequest) {
            if (!result.isOk()) {
                return error(static_cast<int>(result.errorCode()), result.errorMessage(), errHttpCode);
            }
            // 在编译期判断 T 的类型
            return success(extractData(result.value()));
        }

        static QHttpServerResponse fromResult(const Result<void>& result, QHttpServerResponse::StatusCode errHttpCode = QHttpServerResponse::StatusCode::BadRequest) {
            if (result.isOk()) {
                return success();
            }
            return error(static_cast<int>(result.errorCode()), result.errorMessage(), errHttpCode);
        }

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
            QHttpHeaders headers = getCorsHeaders();
            headers.append("Content-Type", "application/json");
            responder.write(QJsonDocument(res).toJson(), headers, httpCode);
        }

        // 统一获取跨域相关的 Header
        static QHttpHeaders getCorsHeaders() {
            QHttpHeaders headers;
            // 允许所有源（实际生产中应限制为特定域名）
            headers.append("Access-Control-Allow-Origin", "*");
            // 允许的跨域请求方法
            headers.append("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            // 允许客户端携带的自定义 Header，增加 Range 支持断点续传
            headers.append("Access-Control-Allow-Headers", "Content-Type, Authorization, Range");
            return headers;
        }

    private:
        // 添加跨域头到 HTTP 响应对象
        static void appendCorsHeaders(QHttpServerResponse& response) {
            response.setHeaders(getCorsHeaders());
        }

        static QJsonValue extractData(const QJsonValue& val) {
            return val;
        }

        template <typename DTO>
        static auto extractData(const DTO& dto) -> decltype(dto.toJson()) {
            return dto.toJson();
        }
    };
}
#endif