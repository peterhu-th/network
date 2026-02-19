#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QUuid>

namespace radar::network {
    DatabaseManager::DatabaseManager(QObject* parent) : QObject(parent) {
        // 使用 UUID (Universally Unique Id) 生成唯一的连接名，防止多线程冲突
        m_connectionName = "AudioRadarDB_" + QUuid::createUuid().toString();
    }

    DatabaseManager::~DatabaseManager() {
        if (m_db.isOpen()) {
            m_db.close();
        }
        QSqlDatabase::removeDatabase(m_connectionName);
    }

    Result<void> DatabaseManager::init(const DatabaseConfig& config) {
        // 保证初始化过程原子性，保护静态资源
        std::lock_guard<std::mutex> lock(m_mutex);

        if (QSqlDatabase::contains(m_connectionName)) {
            m_db = QSqlDatabase::database(m_connectionName);
        } else {
            m_db = QSqlDatabase::addDatabase(config.type, m_connectionName);
        }

        m_db.setHostName(config.host);
        m_db.setPort(config.port);
        m_db.setDatabaseName(config.dbName);
        m_db.setUserName(config.username);
        m_db.setPassword(config.password);

        if (!m_db.open()) {
            return Result<void>::error("Database connection failed: ", ErrorCode::DatabaseInitFailed);
        }

        if (auto res = createTable(); !res.isOk()) {
            return Result<void>::error("Failed to create table: ", ErrorCode::DatabaseInitFailed);
        }

        return Result<void>::ok();
    }

    Result<void> DatabaseManager::createTable() const {
        QSqlQuery query(m_db);
        QString sql = R"(
            CREATE TABLE IF NOT EXISTS audio_records (
                id BIGINT PRIMARY KEY,
                file_path VARCHAR(512) NOT NULL UNIQUE,
                generation_time DATETIME NOT NULL,
                duration_ms INT,
                file_size BIGINT,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        )";
        if (query.exec(sql)) {
            return Result<void>::ok();
        }
        return Result<void>::error("Create table failed: " + query.lastError().text(), ErrorCode::DatabaseInitFailed);
    }

    Result<void> DatabaseManager::insertRecord(const AudioRecord& record) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_db.isOpen()) return Result<void>::error("Database not open", ErrorCode::DatabaseConnectionFailed);

        QSqlQuery query(m_db);
        query.prepare("INSERT INTO audio_records (id, file_path, generation_time, duration_ms, file_size) "
                      "VALUES (:id, :path, :gen_time, :duration, :size)");

        query.bindValue(":id", QVariant::fromValue(record.id));
        query.bindValue(":path", record.filePath);
        query.bindValue(":gen_time", record.generationTime);
        query.bindValue(":duration", record.durationMs);
        query.bindValue(":size", QVariant::fromValue(record.fileSize));

        if (!query.exec()) {
            return Result<void>::error("Insert failed: " + query.lastError().text());
        }

        return Result<void>::ok();
    }

    Result<AudioRecord> DatabaseManager::getRecord(int64_t id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_db.isOpen()) return Result<AudioRecord>::error("Database not open", ErrorCode::DatabaseConnectionFailed);

        QSqlQuery query(m_db);
        query.prepare("SELECT id, file_path, generation_time, duration_ms, file_size, created_at FROM audio_records WHERE id = :id");
        query.bindValue(":id", QVariant::fromValue(id));

        if (!query.exec()) {
            return Result<AudioRecord>::error("Query failed", ErrorCode::DatabaseQueryFailed);
        }
        if (!query.next()) {
            return Result<AudioRecord>::error("Record not found", ErrorCode::RecordNotFound);
        }

        AudioRecord record;
        record.id = query.value("id").toLongLong();
        record.filePath = query.value("file_path").toString();
        record.generationTime = query.value("generation_time").toDateTime();
        record.durationMs = query.value("duration_ms").toInt();
        record.fileSize = query.value("file_size").toLongLong();
        record.createdAt = query.value("created_at").toDateTime();

        return Result<AudioRecord>::ok(record);
    }

    Result<std::vector<AudioRecord>> DatabaseManager::queryRecords(const QDateTime& startTime, const QDateTime& endTime, int limit) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_db.isOpen()) return Result<std::vector<AudioRecord>>::error("Database not open");

        QSqlQuery query(m_db);
        QString sql = "SELECT id, file_path, generation_time, duration_ms, file_size, created_at FROM audio_records WHERE 1=1";

        if (startTime.isValid()) {
            sql += " AND generation_time >= :start";
        } else {
            return Result<std::vector<AudioRecord>>::error("Query failed: invalid start time", ErrorCode::InvalidConfig);
        }
        if (endTime.isValid()) {
            sql += " AND generation_time <= :end";
        } else {
            return Result<std::vector<AudioRecord>>::error("Query failed: invalid end time", ErrorCode::InvalidConfig);
        }
        sql += " ORDER BY generation_time DESC LIMIT :limit";

        query.prepare(sql);
        if (startTime.isValid()) query.bindValue(":start", startTime);
        if (endTime.isValid()) query.bindValue(":end", endTime);
        query.bindValue(":limit", limit);

        if (!query.exec()) {
            return Result<std::vector<AudioRecord>>::error("Query failed: ", ErrorCode::DatabaseQueryFailed);
        }

        std::vector<AudioRecord> records;
        while (query.next()) {
            AudioRecord record;
            record.id = query.value("id").toLongLong();
            record.filePath = query.value("file_path").toString();
            record.generationTime = query.value("generation_time").toDateTime();
            record.durationMs = query.value("duration_ms").toInt();
            record.fileSize = query.value("file_size").toLongLong();
            record.createdAt = query.value("created_at").toDateTime();
            records.push_back(record);
        }

        return Result<std::vector<AudioRecord>>::ok(records);
    }

    Result<bool> DatabaseManager::hasRecord(const QString& filePath) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_db.isOpen()) return Result<bool>::error("Database not open", ErrorCode::DatabaseConnectionFailed);

        QSqlQuery query(m_db);
        query.prepare("SELECT count(*) FROM audio_records WHERE file_path = :path");
        query.bindValue(":path", filePath);

        if (query.exec() && query.next()) {
            return Result<bool>::ok(query.value(0).toInt() > 0);
        }
        return Result<bool>::error("Query failed: " + query.lastError().text(), ErrorCode::DatabaseQueryFailed);
    }
}
