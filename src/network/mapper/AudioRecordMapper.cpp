#include "AudioRecordMapper.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QUuid>

namespace radar::network::mapper {
    AudioRecordMapper::AudioRecordMapper(QObject* parent) : QObject(parent) {
        m_connectionName = "AudioRadarDB_" + QUuid::createUuid().toString();
    }

    AudioRecordMapper::~AudioRecordMapper() {
        if (m_db.isOpen()) {
            m_db.close();
        }
        QSqlDatabase::removeDatabase(m_connectionName);
    }

    Result<void> AudioRecordMapper::init(const DatabaseConfig& config) {
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
            return Result<void>::error("Database connection failed: " + m_db.lastError().text(), ErrorCode::DatabaseInitFailed);
        }

        if (auto res = createTable(); !res.isOk()) {
            return Result<void>::error("Failed to create table: " + res.errorMessage(), ErrorCode::DatabaseInitFailed);
        }

        return Result<void>::ok();
    }

    Result<void> AudioRecordMapper::createTable() const {
        QSqlQuery query(m_db);
        QString sql = R"(
            CREATE TABLE IF NOT EXISTS audio_records (
                id BIGINT PRIMARY KEY,
                file_path VARCHAR(512) NOT NULL UNIQUE,
                generation_time TIMESTAMP NOT NULL,
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

    Result<void> AudioRecordMapper::insertRecord(const AudioRecord& record) {
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

    Result<AudioRecord> AudioRecordMapper::getRecord(int64_t id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_db.isOpen()) return Result<AudioRecord>::error("Database not open", ErrorCode::DatabaseConnectionFailed);

        QSqlQuery query(m_db);
        query.prepare("SELECT id, file_path, generation_time, duration_ms, file_size, created_at FROM audio_records WHERE id = :id");
        query.bindValue(":id", QVariant::fromValue(id));

        if (!query.exec()) {
            return Result<AudioRecord>::error("Query failed: " + query.lastError().text(), ErrorCode::DatabaseQueryFailed);
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

    Result<std::vector<AudioRecord>> AudioRecordMapper::queryRecords(const QDateTime& startTime, const QDateTime& endTime, int limit, int offset) {
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
        sql += " ORDER BY generation_time DESC LIMIT :limit OFFSET :offset";

        query.prepare(sql);
        if (startTime.isValid()) query.bindValue(":start", startTime);
        if (endTime.isValid()) query.bindValue(":end", endTime);
        query.bindValue(":limit", limit);
        query.bindValue(":offset", offset);

        if (!query.exec()) {
            return Result<std::vector<AudioRecord>>::error("Query failed: " + query.lastError().text(), ErrorCode::DatabaseQueryFailed);
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
    
    Result<int> AudioRecordMapper::countRecords(const QDateTime& startTime, const QDateTime& endTime) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_db.isOpen()) return Result<int>::error("Database not open", ErrorCode::DatabaseConnectionFailed);

        QSqlQuery query(m_db);
        QString sql = "SELECT COUNT(*) FROM audio_records WHERE 1=1";

        if (startTime.isValid()) {
            sql += " AND generation_time >= :start";
        }
        if (endTime.isValid()) {
            sql += " AND generation_time <= :end";
        }

        query.prepare(sql);
        if (startTime.isValid()) query.bindValue(":start", startTime);
        if (endTime.isValid()) query.bindValue(":end", endTime);

        if (!query.exec()) {
            return Result<int>::error("Count Query failed: " + query.lastError().text(), ErrorCode::DatabaseQueryFailed);
        }
        
        if (query.next()) {
            return Result<int>::ok(query.value(0).toInt());
        }
        
        return Result<int>::ok(0);
    }

    Result<bool> AudioRecordMapper::hasRecord(const QString& filePath) {
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
