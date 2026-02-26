#ifndef AUDIO_RECORD_SERVICE_H
#define AUDIO_RECORD_SERVICE_H

#include "../mapper/AudioRecordMapper.h"
#include "../../core/Types.h"
#include "../utils/FileIndexer.h"
#include <QObject>
#include <memory>

namespace radar::network::service {
    class AudioRecordService : public QObject {
        Q_OBJECT
    public:
        explicit AudioRecordService(QObject* parent = nullptr);
        ~AudioRecordService() override;

        Result<void> init(const DatabaseConfig& dbConfig, const QString& storagePath);
        void start();
        void stop();

        // 为 Controller 提供服务
        Result<std::vector<AudioRecord>> getRecordsPage(const QDateTime& startTime, const QDateTime& endTime, int limit, int offset);
        Result<int> getTotalCount(const QDateTime& startTime, const QDateTime& endTime);
        Result<AudioRecord> getRecordById(int64_t id);

    private:
        std::shared_ptr<mapper::AudioRecordMapper> m_mapper;
        std::unique_ptr<FileIndexer> m_fileIndexer;
        QString m_storagePath;
    };
}

#endif // AUDIO_RECORD_SERVICE_H
