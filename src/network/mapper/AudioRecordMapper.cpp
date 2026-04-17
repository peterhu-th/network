#include <QThreadPool>
#include <QTcpServer>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include "AudioRecordMapper.h"

namespace radar::network {
    AudioRecordMapper::AudioRecordMapper(QString connectionName, QObject* parent)
                : QObject(parent), m_connectionName(std::move(connectionName)) {
        auto res = createTable();
        if (!res.isOk()) {
            qWarning() << "Init table failed:" << res.errorMessage();
        }
    }

    Result<void> AudioRecordMapper::createTable() const {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName);
        if (!db.isOpen()) return Result<void>::error("Database not open", ErrorCode::DatabaseConnectionFailed);

        QStringList tables = db.tables(QSql::Tables);
        QSqlQuery query(db);
        
        if (!tables.contains("audio_records", Qt::CaseInsensitive)) {
            QString sql = R"(
                CREATE TABLE audio_records (
                    id BIGINT PRIMARY KEY,
                    file_path VARCHAR(512) NOT NULL UNIQUE,
                    generation_time TIMESTAMP NOT NULL,
                    duration INT,
                    file_size BIGINT,
                    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                )
            )";
            if (!query.exec(sql)) {
                return Result<void>::error("Create table failed: " + query.lastError().text(), ErrorCode::DatabaseInitFailed);
            }
        }

        if (!tables.contains("download_logs", Qt::CaseInsensitive)) {
            QString sqlLogs = R"(
                CREATE TABLE download_logs (
                    log_id SERIAL PRIMARY KEY,
                    file_id BIGINT NOT NULL,
                    downloaded_at TIMESTAMP NOT NULL
                )
            )";
            if (!query.exec(sqlLogs)) {
                return Result<void>::error("Create download_logs table failed: " + query.lastError().text(), ErrorCode::DatabaseInitFailed);
            }
        }
        return Result<void>::ok();
    }

    Result<std::vector<std::pair<qint64, QString>>> AudioRecordMapper::getAllFilePaths() const {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName);
        if (!db.isOpen()) {
            return Result<std::vector<std::pair<qint64, QString>>>::error("Database not open", ErrorCode::DatabaseConnectionFailed);
        }

        QSqlQuery query(db);
        if (!query.exec("SELECT id, file_path FROM audio_records")) {
            return Result<std::vector<std::pair<qint64, QString>>>::error("Query all file paths failed: " + query.lastError().text(), ErrorCode::DatabaseQueryFailed);
        }

        std::vector<std::pair<qint64, QString>> results;
        while (query.next()) {
            qint64 id = query.value("id").toLongLong();
            QString path = query.value("file_path").toString();
            results.emplace_back(id, path);
        }

        return Result<std::vector<std::pair<qint64, QString>>>::ok(results);
    }

    Result<void> AudioRecordMapper::insertRecord(const AudioRecord &record) const {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName);
        if (!db.isOpen()) return Result<void>::error("Database not open", ErrorCode::DatabaseConnectionFailed);

        QSqlQuery query(db);
        // 使用键值绑定防止注入：第一次发送骨架锁死指令结构，第二次单独发送数据
        // prepare 用于含有变量的 SQL 语句，声明占位再用 bindValue 填充
        query.prepare("INSERT INTO audio_records (id, file_path, generation_time, duration, file_size) "
                      "VALUES (:id, :file_path, :generation_time, :duration, :size)");
        query.bindValue(":id", QVariant::fromValue(record.id));
        query.bindValue(":file_path", record.filePath);
        query.bindValue(":generation_time", record.generationTime);
        query.bindValue(":duration", record.duration);
        query.bindValue(":size", QVariant::fromValue(record.fileSize));

        if (!query.exec()) {
            return Result<void>::error("Insert failed: " + query.lastError().text(), ErrorCode::DatabaseQueryFailed);
        }
        return Result<void>::ok();
    }

    Result<void> AudioRecordMapper::deleteRecord(qint64 id) const {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName);
        if (!db.isOpen()) {
            return Result<void>::error("Database not open", ErrorCode::DatabaseConnectionFailed);
        }

        QSqlQuery query(db);
        query.prepare("DELETE FROM audio_records WHERE id = :id");
        query.bindValue(":id", QVariant::fromValue(id));

        if (!query.exec()) {
            return Result<void>::error("Delete record failed: " + query.lastError().text(), ErrorCode::DatabaseQueryFailed);
        }

        return Result<void>::ok();
    }

    Result<QString> AudioRecordMapper::getFilePathById(qint64 id) const {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName);
        if (!db.isOpen()) return Result<QString>::error("Database not open", ErrorCode::DatabaseConnectionFailed);

        QSqlQuery query(db);
        query.prepare("SELECT file_path FROM audio_records WHERE id = :id LIMIT 1");
        query.bindValue(":id", QVariant::fromValue(id));

        if (query.exec() && query.next()) {
            return Result<QString>::ok(query.value("file_path").toString());
        }
        return Result<QString>::error("Failed to get file path or record not found", ErrorCode::RecordNotFound);
    }

    Result<std::vector<AudioRecord>> AudioRecordMapper::queryRecords(const QDateTime &startTime, const QDateTime &endTime, const QString& format, int limit, int offset) const {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName);
        if (!db.isOpen()) return Result<std::vector<AudioRecord>>::error("Database not open", ErrorCode::DatabaseConnectionFailed);

        QSqlQuery query(db);
        QString sql = "SELECT id, file_path, generation_time, duration, file_size, created_at FROM audio_records WHERE 1=1";
        if (startTime.isValid()) sql += " AND generation_time >= :start_time";
        if (endTime.isValid()) sql += " AND generation_time <= :end_time";
        if (!format.isEmpty()) sql += " AND file_path ILIKE :format";
        sql += " ORDER BY generation_time DESC LIMIT :limit OFFSET :offset";

        query.prepare(sql);
        if (startTime.isValid()) query.bindValue(":start_time", startTime);
        if (endTime.isValid()) query.bindValue(":end_time", endTime);
        if (!format.isEmpty()) query.bindValue(":format", "%." + format);
        query.bindValue(":limit", limit);
        query.bindValue(":offset", offset);

        if (!query.exec()) return Result<std::vector<AudioRecord>>::error("Query failed: " + query.lastError().text(), ErrorCode::DatabaseQueryFailed);

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

    Result<int> AudioRecordMapper::countRecords(const QDateTime &startTime, const QDateTime &endTime, const QString& format) const {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName);
        if (!db.isOpen()) return Result<int>::error("Database not open", ErrorCode::DatabaseConnectionFailed);

        QSqlQuery query(db);
        // 使用 SQL 内置聚合函数，返回 1 行 1 列数据
        QString sql = "SELECT COUNT(*) FROM audio_records WHERE 1=1";
        if (startTime.isValid()) sql += " AND generation_time >= :start_time";
        if (endTime.isValid()) sql += " AND generation_time <= :end_time";
        if (!format.isEmpty()) sql += " AND file_path ILIKE :format";

        query.prepare(sql);
        if (startTime.isValid()) query.bindValue(":start_time", startTime);
        if (endTime.isValid()) query.bindValue(":end_time", endTime);
        if (!format.isEmpty()) query.bindValue(":format", "%." + format);

        if (!query.exec()) return Result<int>::error("Count query failed: " + query.lastError().text(), ErrorCode::DatabaseQueryFailed);
        if (query.next()) return Result<int>::ok(query.value(0).toInt());
        return Result<int>::ok(0);
    }

    Result<bool> AudioRecordMapper::hasRecord(const QString &filePath) const {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName);
        if (!db.isOpen()) return Result<bool>::error("Database not open", ErrorCode::DatabaseConnectionFailed);

        QSqlQuery query(db);
        query.prepare("SELECT 1 FROM audio_records WHERE file_path = :path LIMIT 1");
        query.bindValue(":path", filePath);

        if (query.exec() && query.next()) {
            return Result<bool>::ok(true);
        }
        return Result<bool>::ok(false);
    }

    Result<void> AudioRecordMapper::insertDownloadLog(qint64 fileId, const QDateTime& time) const {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName);
        if (!db.isOpen()) return Result<void>::error("Database not open", ErrorCode::DatabaseConnectionFailed);

        QSqlQuery query(db);
        query.prepare("INSERT INTO download_logs (file_id, downloaded_at) VALUES (:file_id, :downloaded_at)");
        query.bindValue(":file_id", QVariant::fromValue(fileId));
        query.bindValue(":downloaded_at", time);

        if (!query.exec()) {
            return Result<void>::error("Insert download log failed: " + query.lastError().text(), ErrorCode::DatabaseQueryFailed);
        }
        return Result<void>::ok();
    }
}