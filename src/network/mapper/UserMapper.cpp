#include "UserMapper.h"
#include "../../core/Logger.h"

namespace radar::network {

    UserMapper::UserMapper(QString connectionName, QObject* parent)
        : QObject(parent), m_connectionName(std::move(connectionName)) {
        if (auto res = createTable(); !res.isOk()) {
            LOG_ERROR("Database", "Failed to create users table: " + res.errorMessage());
        }
    }

    Result<void> UserMapper::createTable() const {
        QSqlDatabase db = utils::DbUtils::getConnection(m_connectionName);
        if (!db.isValid() || !db.isOpen()) {
            return Result<void>::error("Database connection is not open", ErrorCode::DatabaseQueryFailed);
        }

        QSqlQuery query(db);
        const QString sql = R"(
            CREATE TABLE IF NOT EXISTS users (
                id VARCHAR(64) NOT NULL PRIMARY KEY,
                email VARCHAR(255) NOT NULL UNIQUE,
                password_hash VARCHAR(255) NOT NULL,
                role SMALLINT NOT NULL DEFAULT 0,
                status SMALLINT NOT NULL DEFAULT 1,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        )";

        if (!query.exec(sql)) {
            return Result<void>::error(query.lastError().text(), ErrorCode::DatabaseQueryFailed);
        }

        return Result<void>::ok();
    }

    Result<void> UserMapper::insert(const UserEntity& user) const {
        QSqlDatabase db = utils::DbUtils::getConnection(m_connectionName);
        QSqlQuery query(db);

        query.prepare("INSERT INTO users (id, email, password_hash, role, status, created_at, updated_at) "
                      "VALUES (:id, :email, :password_hash, :role, :status, :created_at, :updated_at)");
        query.bindValue(":id", user.id);
        query.bindValue(":email", user.email);
        query.bindValue(":password_hash", user.passwordHash);
        query.bindValue(":role", user.role);
        query.bindValue(":status", user.status);
        query.bindValue(":created_at", user.createdAt);
        query.bindValue(":updated_at", user.updatedAt);

        if (!query.exec()) {
            return Result<void>::error(query.lastError().text(), ErrorCode::DatabaseQueryFailed);
        }
        return Result<void>::ok();
    }

    Result<std::optional<UserEntity>> UserMapper::findByEmail(const QString& email) const {
        QSqlDatabase db = utils::DbUtils::getConnection(m_connectionName);
        QSqlQuery query(db);

        query.prepare("SELECT id, email, password_hash, role, status, created_at, updated_at "
                      "FROM users WHERE email = :email LIMIT 1");
        query.bindValue(":email", email);

        if (!query.exec()) {
            return Result<std::optional<UserEntity>>::error(query.lastError().text(), ErrorCode::DatabaseQueryFailed);
        }

        if (query.next()) {
            UserEntity user;
            user.id = query.value(0).toString();
            user.email = query.value(1).toString();
            user.passwordHash = query.value(2).toString();
            user.role = query.value(3).toInt();
            user.status = query.value(4).toInt();
            // Handling TIMESTAMP properly if returned as string or QDateTime
            user.createdAt = query.value(5).toDateTime();
            user.updatedAt = query.value(6).toDateTime();
            return Result<std::optional<UserEntity>>::ok(user);
        }
        
        return Result<std::optional<UserEntity>>::ok(std::nullopt);
    }

    Result<std::optional<UserEntity>> UserMapper::findById(const QString& id) const {
        QSqlDatabase db = utils::DbUtils::getConnection(m_connectionName);
        QSqlQuery query(db);

        query.prepare("SELECT id, email, password_hash, role, status, created_at, updated_at "
                      "FROM users WHERE id = :id LIMIT 1");
        query.bindValue(":id", id);

        if (!query.exec()) {
            return Result<std::optional<UserEntity>>::error(query.lastError().text(), ErrorCode::DatabaseQueryFailed);
        }

        if (query.next()) {
            UserEntity user;
            user.id = query.value(0).toString();
            user.email = query.value(1).toString();
            user.passwordHash = query.value(2).toString();
            user.role = query.value(3).toInt();
            user.status = query.value(4).toInt();
            user.createdAt = query.value(5).toDateTime();
            user.updatedAt = query.value(6).toDateTime();
            return Result<std::optional<UserEntity>>::ok(user);
        }

        return Result<std::optional<UserEntity>>::ok(std::nullopt);
    }

    Result<std::vector<UserDTO>> UserMapper::findGuests(int limit, int offset) const {
        QSqlDatabase db = utils::DbUtils::getConnection(m_connectionName);
        QSqlQuery query(db);

        query.prepare("SELECT id, email, role, status, created_at "
                      "FROM users WHERE role = 0 ORDER BY created_at DESC LIMIT :limit OFFSET :offset");
        query.bindValue(":limit", limit);
        query.bindValue(":offset", offset);

        if (!query.exec()) {
            return Result<std::vector<UserDTO>>::error(query.lastError().text(), ErrorCode::DatabaseQueryFailed);
        }

        std::vector<UserDTO> guests;
        while (query.next()) {
            UserDTO dto;
            dto.id = query.value(0).toString();
            dto.email = query.value(1).toString();
            dto.role = query.value(2).toInt();
            dto.status = query.value(3).toInt();
            dto.createdAt = query.value(4).toDateTime();
            guests.push_back(dto);
        }

        return Result<std::vector<UserDTO>>::ok(guests);
    }

    Result<void> UserMapper::updateStatus(const QString& id, int status) const {
        QSqlDatabase db = utils::DbUtils::getConnection(m_connectionName);
        QSqlQuery query(db);

        query.prepare("UPDATE users SET status = :status, updated_at = CURRENT_TIMESTAMP WHERE id = :id");
        query.bindValue(":status", status);
        query.bindValue(":id", id);

        if (!query.exec()) {
            return Result<void>::error(query.lastError().text(), ErrorCode::DatabaseQueryFailed);
        }
        return Result<void>::ok();
    }

    Result<int> UserMapper::countUsers() const {
        QSqlDatabase db = utils::DbUtils::getConnection(m_connectionName);
        QSqlQuery query(db);

        query.prepare("SELECT COUNT(*) FROM users");
        if (!query.exec() || !query.next()) {
            return Result<int>::error(query.lastError().text(), ErrorCode::DatabaseQueryFailed);
        }

        return Result<int>::ok(query.value(0).toInt());
    }

}
