#ifndef USER_MAPPER_H
#define USER_MAPPER_H

#include <vector>
#include <optional>
#include <QSqlQuery>
#include "../../core/Types.h"
#include "NetworkDTO.h"
#include "../utils/dbUtils.h"

namespace radar::network {
    class UserMapper : public QObject {
        Q_OBJECT

    public:
        explicit UserMapper(QString connectionName, QObject* parent = nullptr);
        ~UserMapper() override = default;

        [[nodiscard]] Result<void> insert(const UserEntity& user) const;
        [[nodiscard]] Result<std::optional<UserEntity>> findByEmail(const QString& email) const;
        [[nodiscard]] Result<std::optional<UserEntity>> findById(const QString& id) const;
        [[nodiscard]] Result<std::vector<UserDTO>> findGuests(int limit = 100, int offset = 0) const;
        [[nodiscard]] Result<void> updateStatus(const QString& id, int status) const;
        [[nodiscard]] Result<int> countUsers() const;
        
        [[nodiscard]] Result<void> incrementFailedAttempts(const QString& id) const;
        [[nodiscard]] Result<void> resetFailedAttempts(const QString& id) const;
        [[nodiscard]] Result<void> lockAccount(const QString& id, int minutes) const;
        [[nodiscard]] Result<void> updatePassword(const QString& email, const QString& newPasswordHash) const;

    private:
        QString m_connectionName;
        [[nodiscard]] Result<void> createTable() const;
    };
}

#endif // USER_MAPPER_H
