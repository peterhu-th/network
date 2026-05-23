#ifndef AUTH_CONTROLLER_H
#define AUTH_CONTROLLER_H

#include <memory>
#include <QHttpServer>
#include "../service/AuthService.h"

namespace radar::network {
    class AuthController : public QObject {
        Q_OBJECT

    public:
        explicit AuthController(std::shared_ptr<AuthService> authService, QObject* parent = nullptr);
        ~AuthController() override = default;

        void setupRoutes(QHttpServer& httpServer, const QString& jwtSecret) const;

    private:
        std::shared_ptr<AuthService> m_authService;

        [[nodiscard]] QHttpServerResponse handleSendCode(const QHttpServerRequest& request) const;
        [[nodiscard]] QHttpServerResponse handleRegister(const QHttpServerRequest& request) const;
        [[nodiscard]] QHttpServerResponse handleLogin(const QHttpServerRequest& request) const;
    };
}

#endif // AUTH_CONTROLLER_H
