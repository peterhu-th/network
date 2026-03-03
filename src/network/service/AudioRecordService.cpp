#include <QDebug>
#include "AudioRecordService.h"

namespace radar::network {
    AudioRecordService::AudioRecordService(QObject *parent): QObject(parent) {
        m_mapper = std::make_shared<AudioRecordMapper>();
    }

    AudioRecordService::~AudioRecordService() {
        stop();
    }

    Result<void> AudioRecordService::init(const DatabaseConfig& dbConfig, const QString& storagePath) {
        m_storagePath = storagePath;
        if (auto res = m_mapper->init(dbConfig); !res.isOk()) {
            return res;
        }
        m_fileIndexer = std::make_unique<FileIndexer>(m_mapper.get(), this);
        return Result<void>::ok();
    }

    void AudioRecordService::start() {
        if (m_fileIndexer) {
            auto res = m_fileIndexer->start(m_storagePath);
            if (!res.isOk()) {
                qWarning() << "FileIndexer start failed: " << res.errorMessage();
            }
        }
    }

    void AudioRecordService::stop() {}

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