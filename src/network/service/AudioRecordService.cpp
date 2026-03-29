#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include "AudioRecordService.h"

namespace radar::network {
    AudioRecordService::AudioRecordService(QObject *parent): QObject(parent) {}

    // 建立数据库连接池
    Result<void> AudioRecordService::init(const DatabaseConfig& dbConfig, const QString& storagePath) {
        m_storagePath = storagePath;
        m_globalConnectionName = "Audio_GlobalPool";
        QSqlDatabase db;
        if (!QSqlDatabase::contains(m_globalConnectionName)) {
            db = QSqlDatabase::addDatabase(dbConfig.type, m_globalConnectionName);
        } else {
            db = QSqlDatabase::database(m_globalConnectionName);
        }
        db.setHostName(dbConfig.host);
        db.setPort(dbConfig.port);
        db.setDatabaseName(dbConfig.dbName);
        db.setUserName(dbConfig.username);
        db.setPassword(dbConfig.password);
        if (!db.open()) {
            return Result<void>::error("Service DB init failed: " + db.lastError().text(), ErrorCode::DatabaseInitFailed);
        }
        m_mapper = std::make_shared<AudioRecordMapper>(m_globalConnectionName);
        m_fileIndexer = std::make_unique<FileIndexer>(m_mapper.get(), this);
        return Result<void>::ok();
    }

    void AudioRecordService::start() const {
        if (m_fileIndexer) {
            auto res = m_fileIndexer->start(m_storagePath);
            if (!res.isOk()) {
                qWarning() << "FileIndexer start failed: " << res.errorMessage();
            }
        }
    }

    Result<std::vector<AudioRecord> > AudioRecordService::getRecordPage(const QDateTime &startTime, const QDateTime &endTime, int limit, int offset) const {
        return m_mapper->queryRecords(startTime, endTime, limit, offset);
    }

    Result<int> AudioRecordService::getTotalCount(const QDateTime &startTime, const QDateTime &endTime) const {
        return m_mapper->countRecords(startTime, endTime);
    }

    Result<AudioRecord> AudioRecordService::getRecordById(int64_t id) const{
        return m_mapper->getRecord(id);
    }
}