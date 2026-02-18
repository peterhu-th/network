#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QUuid>

namespace radar {
namespace network {

DatabaseManager::DatabaseManager(QObject* parent) : QObject(parent) {
    // 使用 UUID 生成唯一的连接名，防止多线程冲突
    m_connectionName = "AudioRadarDB_" + QUuid::createUuid().toString();
}

DatabaseManager::~DatabaseManager() {
    if (m_db.isOpen()) {
        m_db.close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

Result<void> DatabaseManager::init(const DatabaseConfig& config) {
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
        return Result<void>::error("Database connection failed: " + m_db.lastError().text());
    }

    if (!createTable()) {
        return Result<void>::error("Failed to create table: " + m_db.lastError().text());
    }

    return Result<void>::ok();
}

bool DatabaseManager::createTable() {
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
    return query.exec(sql);
}

Result<void> DatabaseManager::insertRecord(const AudioRecord& record) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db.isOpen()) return Result<void>::error("Database not open");

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
    if (!m_db.isOpen()) return Result<AudioRecord>::error("Database not open");

    QSqlQuery query(m_db);
    query.prepare("SELECT id, file_path, generation_time, duration_ms, file_size, created_at FROM audio_records WHERE id = :id");
    query.bindValue(":id", QVariant::fromValue(id));

    if (!query.exec() || !query.next()) {
        return Result<AudioRecord>::error("Record not found");
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
    }
    if (endTime.isValid()) {
        sql += " AND generation_time <= :end";
    }
    sql += " ORDER BY generation_time DESC LIMIT :limit";

    query.prepare(sql);
    if (startTime.isValid()) query.bindValue(":start", startTime);
    if (endTime.isValid()) query.bindValue(":end", endTime);
    query.bindValue(":limit", limit);

    if (!query.exec()) {
        return Result<std::vector<AudioRecord>>::error("Query failed: " + query.lastError().text());
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

bool DatabaseManager::hasRecord(const QString& filePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db.isOpen()) return false;

    QSqlQuery query(m_db);
    query.prepare("SELECT count(*) FROM audio_records WHERE file_path = :path");
    query.bindValue(":path", filePath);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}

} // namespace network
} // namespace radar
