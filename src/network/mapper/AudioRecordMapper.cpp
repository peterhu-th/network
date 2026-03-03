#include <QSqlQuery>
#include <QUuid>
#include <QSqlError>
#include "AudioRecordMapper.h"

namespace radar::network {
    AudioRecordMapper::AudioRecordMapper() {
        m_connectionName = "AudioRadarDB_" + QUuid::createUuid().toString();
    }

    AudioRecordMapper::~AudioRecordMapper() {
        if (m_db.isOpen()) {
            m_db.close();
        }
        QSqlDatabase::removeDatabase(m_connectionName);
    }

    Result<void> AudioRecordMapper::init(const DatabaseConfig &config) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (QSqlDatabase::contains(m_connectionName)) {
            m_db = QSqlDatabase::addDatabase(m_connectionName);
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
        if (!createTable().isOk()) {
            return Result<void>::error("Failed to create table: " + createTable().errorMessage(), ErrorCode::DatabaseInitFailed);
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
                duration INT,
                file_size BIGINT,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        )";
        if (query.exec(sql)) {
            return Result<void>::ok();
        }
        return Result<void>::error("Create table failed: " + query.lastError().text(), ErrorCode::DatabaseInitFailed);
    }

    Result<void> AudioRecordMapper::insertRecord(const AudioRecord &record) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_db.isOpen()) {
            return Result<void>::error("Database not open", ErrorCode::DatabaseConnectionFailed);
        }
        QSqlQuery query(m_db);
        query.prepare("INSERT INTO audio_records (id, file_path, generation_time, duration, file_size) "
            "VALUES (:id, :file_path, :generation_time, :duration, :size)");
        query.bindValue(":id", QVariant::fromValue(record.id));
        query.bindValue(":file_path", record.filePath);
        query.bindValue(":generation_time", record.generationTime);
        query.bindValue(":duration", QVariant::fromValue(record.duration));
        query.bindValue(":size", QVariant::fromValue(record.fileSize));
        if (!query.exec()) {
            return Result<void>::error("Insert failed: " + query.lastError().text(), ErrorCode::DatabaseQueryFailed);
        }
        return Result<void>::ok();
    }

    Result<AudioRecord> AudioRecordMapper::getRecord(int64_t id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_db.isOpen()) {
            return Result<AudioRecord>::error("Database not open", ErrorCode::DatabaseConnectionFailed);
        }
        QSqlQuery query(m_db);
        query.prepare("SELECT id ,file_path, generation_time, duration, file_size, created_at FROM audio_records WHERE id = :id");
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
        record.duration = query.value("duration").toInt();
        record.fileSize = query.value("file_size").toLongLong();
        record.createdAt = query.value("created_at").toDateTime();
        return Result<AudioRecord>::ok(record);
    }

    Result<QString> AudioRecordMapper::getFilePathById(qint64 id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_db.isOpen()) {
            return Result<QString>::error("Database not open", ErrorCode::DatabaseConnectionFailed);
        }
        QSqlQuery query(m_db);
        query.prepare("SELECT file_path FROM audio_records WHERE id = :id LIMIT 1");
        query.bindValue(":id", id);
        if (query.exec() && query.next()) {
            return Result<QString>::ok(query.value("file_path").toString());
        }
        return Result<QString>::error("Failed to get file path", ErrorCode::DatabaseQueryFailed);
    }

    Result<std::vector<AudioRecord> > AudioRecordMapper::queryRecords(const QDateTime &startTime, const QDateTime &endTime, int limit, int offset) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_db.isOpen()) {
            return Result<std::vector<AudioRecord>>::error("Database not open", ErrorCode::DatabaseConnectionFailed);
        }
        QSqlQuery query(m_db);
        QString sql = "SELECT id, file_path, generation_time, duration, file_size, created_at FROM audio_records WHERE 1=1";
        if (startTime.isValid()) sql += " AND generation_time >= :start_time";
        if (endTime.isValid()) sql += " AND generation_time <= :end_time";
        // 按照时间顺序倒序排列
        sql += " ORDER BY generation_time DESC LIMIT :limit OFFSET :offset";
        query.prepare(sql);
        if (startTime.isValid()) query.bindValue(":start_time", startTime);
        if (startTime.isValid()) query.bindValue(":end_time", endTime);
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
            record.duration = query.value("duration").toInt();
            record.fileSize = query.value("file_size").toLongLong();
            record.createdAt = query.value("created_at").toDateTime();
            records.push_back(record);
        }
        return Result<std::vector<AudioRecord>>::ok(records);
    }

    Result<int> AudioRecordMapper::countRecords(const QDateTime &startTime, const QDateTime &endTime) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_db.isOpen()) {
            return Result<int>::error("Database not open", ErrorCode::DatabaseConnectionFailed);
        }
        QSqlQuery query(m_db);
        QString sql = "SELECT COUNT(*) FROM audio_records WHERE 1=1";
        if (startTime.isValid()) sql += " AND generation_time >= :start_time";
        if (endTime.isValid()) sql += " AND generation_time <= :end_time";
        query.prepare(sql);
        if (startTime.isValid()) query.bindValue(":start_time", startTime);
        if (endTime.isValid()) query.bindValue(":end_time", endTime);
        if (!query.exec()) {
            return Result<int>::error("Count query failed: " + query.lastError().text(), ErrorCode::DatabaseQueryFailed);
        }
        if (query.next()) {
            return Result<int>::ok(query.value(0).toInt());
        }
        return Result<int>::ok(0);
    }

    Result<bool> AudioRecordMapper::hasRecord(const QString &filePath) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_db.isOpen()) {
            return Result<bool>::error("Database not open", ErrorCode::DatabaseConnectionFailed);
        }
        QSqlQuery query(m_db);
        query.prepare("SELECT 1 FROM audio_records WHERE file_path = :path LIMIT 1");
        query.bindValue(":path", filePath);
        if (query.exec() && query.next()) {
            return Result<bool>::ok(true);
        }
        return Result<bool>::ok(false);
    }
}