#ifndef AUDIO_RECORD_SERVICE_H
#define AUDIO_RECORD_SERVICE_H

#include <QObject>
#include <memory>
#include <vector>
#include "../utils/FileIndexer.h"
#include "mapper/AudioRecordMapper.h"

namespace radar::network {
    class AudioRecordService : public QObject {
        Q_OBJECT
    public:
        explicit AudioRecordService(QObject *parent = nullptr);
        ~AudioRecordService() override;
        [[nodiscard]] Result<void> init(const DatabaseConfig& dbConfig, const QString& storagePath);
        void start();
        void stop();
        [[nodiscard]] Result<std::vector<AudioRecord>> getRecordPage(const QDateTime& startTime, const QDateTime& endTime, int limit, int offset) const;
        [[nodiscard]] Result<int> getTotalCount(const QDateTime& startTime, const QDateTime& endTime) const;
        [[nodiscard]] Result<AudioRecord> getRecordById(int64_t id) const;
    private:
        std::shared_ptr<AudioRecordMapper> m_mapper;
        std::unique_ptr<FileIndexer> m_fileIndexer;
        QString m_storagePath;
    };
}

#endif