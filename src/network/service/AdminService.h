#ifndef ADMIN_SERVICE_H
#define ADMIN_SERVICE_H

#include <QString>
#include <vector>
#include <memory>
#include "../../core/Types.h"
#include "../mapper/UserMapper.h"

namespace radar::network {
    class AdminService {
    public:
        AdminService(std::shared_ptr<UserMapper> userMapper);
        ~AdminService() = default;

        // 获取所有的普通用户 (访客) 列表，不包含密码信息
        [[nodiscard]] Result<std::vector<UserDTO>> getGuestList(int limit = 100, int offset = 0) const;

        // 撤销/封禁一个普通用户的权限
        [[nodiscard]] Result<void> revokeGuest(const QString& userId) const;

    private:
        std::shared_ptr<UserMapper> m_userMapper;
    };
}

#endif // ADMIN_SERVICE_H
