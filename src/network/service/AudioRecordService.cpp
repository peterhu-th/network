#include "AudioRecordService.h"
#include <QDebug>

namespace radar::network::service {

    AudioRecordService::AudioRecordService(QObject* parent) : QObject(parent) {
        m_mapper = std::make_shared<mapper::AudioRecordMapper>();
        // 注意：这里 FileIndexer 需要修改接受 AudioRecordMapper 指针
        // 我会在之后修改 FileIndexer 的头文件
    }

    AudioRecordService::~AudioRecordService() {
        stop();
    }

    Result<void> AudioRecordService::init(const DatabaseConfig& dbConfig, const QString& storagePath) {
        m_storagePath = storagePath;
        if (auto res = m_mapper->init(dbConfig); !res.isOk()) {
            return res;
        }
        
        m_fileIndexer = std::make_unique<FileIndexer>(m_mapper.get());
        
        return Result<void>::ok();
    }

    void AudioRecordService::start() {
        if (m_fileIndexer) {
            auto res = m_fileIndexer->start(m_storagePath);
            if (!res.isOk()) {
                qWarning() << "FileIndexer start failed:" << res.errorMessage();
            }
        }
    }

    void AudioRecordService::stop() {
        // Any specific stop logic
    }

    Result<std::vector<AudioRecord>> AudioRecordService::getRecordsPage(const QDateTime& startTime, const QDateTime& endTime, int limit, int offset) {
        return m_mapper->queryRecords(startTime, endTime, limit, offset);
    }

    Result<int> AudioRecordService::getTotalCount(const QDateTime& startTime, const QDateTime& endTime) {
        return m_mapper->countRecords(startTime, endTime);
    }

    Result<AudioRecord> AudioRecordService::getRecordById(int64_t id) {
        return m_mapper->getRecord(id);
    }

}
