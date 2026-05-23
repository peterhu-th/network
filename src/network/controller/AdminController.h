#ifndef ADMIN_CONTROLLER_H
#define ADMIN_CONTROLLER_H

#include <memory>
#include <QHttpServer>
#include "../service/AdminService.h"
#include "../service/AuthService.h"
#include "../mapper/UserMapper.h"

namespace radar::network {
    class AdminController : public QObject {
        Q_OBJECT

    public:
        explicit AdminController(std::shared_ptr<AdminService> adminService, std::shared_ptr<AuthService> authService, QObject* parent = nullptr);
        ~AdminController() override = default;

        void setupRoutes(QHttpServer& httpServer, const QString& jwtSecret,
                         std::shared_ptr<UserMapper> userMapper) const;

    private:
        std::shared_ptr<AdminService> m_adminService;
        std::shared_ptr<AuthService> m_authService;

        [[nodiscard]] QHttpServerResponse handleCreateAdmin(const QHttpServerRequest& request) const;
        [[nodiscard]] QHttpServerResponse handleGetGuests(const QHttpServerRequest& request) const;
        [[nodiscard]] QHttpServerResponse handleRevokeGuest(const QHttpServerRequest& request, const QString& userId) const;
    };
}

#endif // ADMIN_CONTROLLER_H
