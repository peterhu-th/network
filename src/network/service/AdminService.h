#ifndef ADMIN_SERVICE_H
#define ADMIN_SERVICE_H

#include "../../core/Types.h"
#include "../mapper/UserMapper.h"

namespace radar::network {
    class AdminService {
    public:
        explicit AdminService(std::shared_ptr<UserMapper> userMapper);
        ~AdminService() = default;

        [[nodiscard]] Result<std::vector<UserDTO>> getGuestList(int limit = 100, int offset = 0) const;
        [[nodiscard]] Result<void> revokeGuest(const QString& userId) const;

    private:
        std::shared_ptr<UserMapper> m_userMapper;
    };
}

#endif
