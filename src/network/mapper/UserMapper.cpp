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
                failed_attempts SMALLINT NOT NULL DEFAULT 0,
                locked_until TIMESTAMP,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        )";

        if (!query.exec(sql)) {
            LOG_ERROR("Database", "Create users table failed: " + query.lastError().text());
            return Result<void>::error("Database query failed", ErrorCode::DatabaseQueryFailed);
        }

        query.exec("ALTER TABLE users ADD COLUMN failed_attempts SMALLINT NOT NULL DEFAULT 0");
        query.exec("ALTER TABLE users ADD COLUMN locked_until TIMESTAMP");

        return Result<void>::ok();
    }

    Result<void> UserMapper::insert(const UserEntity& user) const {
        QSqlDatabase db = utils::DbUtils::getConnection(m_connectionName);
        QSqlQuery query(db);

        query.prepare("INSERT INTO users (id, email, password_hash, role, status, failed_attempts, locked_until, created_at, updated_at) "
                      "VALUES (:id, :email, :password_hash, :role, :status, :failed_attempts, :locked_until, :created_at, :updated_at)");
        query.bindValue(":id", user.id);
        query.bindValue(":email", user.email);
        query.bindValue(":password_hash", user.passwordHash);
        query.bindValue(":role", user.role);
        query.bindValue(":status", user.status);
        query.bindValue(":failed_attempts", user.failedAttempts);
        query.bindValue(":locked_until", user.lockedUntil);
        query.bindValue(":created_at", user.createdAt);
        query.bindValue(":updated_at", user.updatedAt);

        if (!query.exec()) {
            LOG_ERROR("Database", "Insert user failed: " + query.lastError().text());
            return Result<void>::error("Database query failed", ErrorCode::DatabaseQueryFailed);
        }
        return Result<void>::ok();
    }

    Result<std::optional<UserEntity>> UserMapper::findByEmail(const QString& email) const {
        QSqlDatabase db = utils::DbUtils::getConnection(m_connectionName);
        QSqlQuery query(db);

        query.prepare("SELECT id, email, password_hash, role, status, failed_attempts, locked_until, created_at, updated_at "
                      "FROM users WHERE email = :email LIMIT 1");
        query.bindValue(":email", email);

        if (!query.exec()) {
            LOG_ERROR("Database", "FindByEmail failed: " + query.lastError().text());
            return Result<std::optional<UserEntity>>::error("Database query failed", ErrorCode::DatabaseQueryFailed);
        }

        if (query.next()) {
            UserEntity user;
            user.id = query.value(0).toString();
            user.email = query.value(1).toString();
            user.passwordHash = query.value(2).toString();
            user.role = query.value(3).toInt();
            user.status = query.value(4).toInt();
            user.failedAttempts = query.value(5).toInt();
            user.lockedUntil = query.value(6).toDateTime();
            user.createdAt = query.value(7).toDateTime();
            user.updatedAt = query.value(8).toDateTime();
            return Result<std::optional<UserEntity>>::ok(user);
        }
        
        return Result<std::optional<UserEntity>>::ok(std::nullopt);
    }

    Result<std::optional<UserEntity>> UserMapper::findById(const QString& id) const {
        QSqlDatabase db = utils::DbUtils::getConnection(m_connectionName);
        QSqlQuery query(db);

        query.prepare("SELECT id, email, password_hash, role, status, failed_attempts, locked_until, created_at, updated_at "
                      "FROM users WHERE id = :id LIMIT 1");
        query.bindValue(":id", id);

        if (!query.exec()) {
            LOG_ERROR("Database", "FindById failed: " + query.lastError().text());
            return Result<std::optional<UserEntity>>::error("Database query failed", ErrorCode::DatabaseQueryFailed);
        }

        if (query.next()) {
            UserEntity user;
            user.id = query.value(0).toString();
            user.email = query.value(1).toString();
            user.passwordHash = query.value(2).toString();
            user.role = query.value(3).toInt();
            user.status = query.value(4).toInt();
            user.failedAttempts = query.value(5).toInt();
            user.lockedUntil = query.value(6).toDateTime();
            user.createdAt = query.value(7).toDateTime();
            user.updatedAt = query.value(8).toDateTime();
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
            LOG_ERROR("Database", "FindGuests failed: " + query.lastError().text());
            return Result<std::vector<UserDTO>>::error("Database query failed", ErrorCode::DatabaseQueryFailed);
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

        query.prepare("UPDATE users SET status = :status, updated_at = :updated_at WHERE id = :id");
        query.bindValue(":status", status);
        query.bindValue(":updated_at", QDateTime::currentDateTimeUtc());
        query.bindValue(":id", id);

        if (!query.exec()) {
            LOG_ERROR("Database", "UpdateStatus failed: " + query.lastError().text());
            return Result<void>::error("Database query failed", ErrorCode::DatabaseQueryFailed);
        }
        return Result<void>::ok();
    }

    Result<int> UserMapper::countUsers() const {
        QSqlDatabase db = utils::DbUtils::getConnection(m_connectionName);
        QSqlQuery query(db);

        query.prepare("SELECT COUNT(*) FROM users");
        if (!query.exec() || !query.next()) {
            LOG_ERROR("Database", "CountUsers failed: " + query.lastError().text());
            return Result<int>::error("Database query failed", ErrorCode::DatabaseQueryFailed);
        }

        return Result<int>::ok(query.value(0).toInt());
    }

    Result<void> UserMapper::incrementFailedAttempts(const QString& id) const {
        QSqlDatabase db = utils::DbUtils::getConnection(m_connectionName);
        QSqlQuery query(db);

        query.prepare("UPDATE users SET failed_attempts = failed_attempts + 1, updated_at = :updated_at WHERE id = :id");
        query.bindValue(":updated_at", QDateTime::currentDateTimeUtc());
        query.bindValue(":id", id);

        if (!query.exec()) {
            LOG_ERROR("Database", "IncrementFailedAttempts failed: " + query.lastError().text());
            return Result<void>::error("Database query failed", ErrorCode::DatabaseQueryFailed);
        }
        return Result<void>::ok();
    }

    Result<void> UserMapper::resetFailedAttempts(const QString& id) const {
        QSqlDatabase db = utils::DbUtils::getConnection(m_connectionName);
        QSqlQuery query(db);

        query.prepare("UPDATE users SET failed_attempts = 0, locked_until = NULL, updated_at = :updated_at WHERE id = :id");
        query.bindValue(":updated_at", QDateTime::currentDateTimeUtc());
        query.bindValue(":id", id);

        if (!query.exec()) {
            LOG_ERROR("Database", "ResetFailedAttempts failed: " + query.lastError().text());
            return Result<void>::error("Database query failed", ErrorCode::DatabaseQueryFailed);
        }
        return Result<void>::ok();
    }

    Result<void> UserMapper::lockAccount(const QString& id, int minutes) const {
        QSqlDatabase db = utils::DbUtils::getConnection(m_connectionName);
        QSqlQuery query(db);

        query.prepare("UPDATE users SET locked_until = :locked_until, updated_at = :updated_at WHERE id = :id");
        QDateTime lockedUntil = QDateTime::currentDateTimeUtc().addSecs(minutes * 60);
        query.bindValue(":locked_until", lockedUntil);
        query.bindValue(":updated_at", QDateTime::currentDateTimeUtc());
        query.bindValue(":id", id);

        if (!query.exec()) {
            LOG_ERROR("Database", "LockAccount failed: " + query.lastError().text());
            return Result<void>::error("Database query failed", ErrorCode::DatabaseQueryFailed);
        }
        return Result<void>::ok();
    }

    Result<void> UserMapper::updatePassword(const QString& email, const QString& newPasswordHash) const {
        QSqlDatabase db = utils::DbUtils::getConnection(m_connectionName);
        QSqlQuery query(db);

        query.prepare("UPDATE users SET password_hash = :hash, failed_attempts = 0, locked_until = NULL, updated_at = :updated_at WHERE email = :email");
        query.bindValue(":hash", newPasswordHash);
        query.bindValue(":updated_at", QDateTime::currentDateTimeUtc());
        query.bindValue(":email", email);

        if (!query.exec()) {
            LOG_ERROR("Database", "UpdatePassword failed: " + query.lastError().text());
            return Result<void>::error("Database query failed", ErrorCode::DatabaseQueryFailed);
        }
        return Result<void>::ok();
    }

}
