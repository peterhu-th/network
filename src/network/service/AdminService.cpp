#include "AdminService.h"

namespace radar::network {

    AdminService::AdminService(std::shared_ptr<UserMapper> userMapper)
        : m_userMapper(std::move(userMapper)) {}

    Result<std::vector<UserDTO>> AdminService::getGuestList(int limit, int offset) const {
        return m_userMapper->findGuests(limit, offset);
    }

    Result<void> AdminService::revokeGuest(const QString& userId) const {
        auto userRes = m_userMapper->findById(userId);
        if (!userRes.isOk()) {
            return Result<void>::error(userRes.errorMessage(), userRes.errorCode());
        }
        if (!userRes.value().has_value()) {
            return Result<void>::error("User not found", ErrorCode::RecordNotFound);
        }
        if (userRes.value().value().role != 0) {
            return Result<void>::error("Only guest users can be revoked", ErrorCode::InvalidParam);
        }
        return m_userMapper->updateStatus(userId, 0);
    }
}
