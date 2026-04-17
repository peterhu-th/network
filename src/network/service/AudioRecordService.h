#ifndef AUDIO_RECORD_SERVICE_H
#define AUDIO_RECORD_SERVICE_H

#include <QObject>
#include <memory>
#include <vector>
#include <QCryptographicHash>
#include <QReadWriteLock>
#include "../utils/FileIndexer.h"
#include "../NetworkDTO.h"
#include "../mapper/AudioRecordMapper.h"

namespace radar::network {
    struct JobStatus {
        QString taskId;
        QString status;      // "Pending", "Processing", "Completed", "Failed"
        QString resultPath;
        qint64 createdAt;
    };

    class AudioRecordService : public QObject {
        Q_OBJECT

    public:
        explicit AudioRecordService(QObject *parent = nullptr);
        ~AudioRecordService() override = default;
        [[nodiscard]] Result<void> init(const DatabaseConfig& dbConfig, const NetworkConfig& netConfig);
        void start() const;
        [[nodiscard]] Result<void> forceScan() const;

        static void stop();
        [[nodiscard]] Result<int> getTotalCount(const QDateTime& startTime, const QDateTime& endTime, const QString& format = "") const;
        [[nodiscard]] Result<std::vector<AudioRecordDTO>> getRecordPage(const QDateTime& startTime, const QDateTime& endTime, const QString& format = "", int limit = 100, int offset = 0) const;
        [[nodiscard]] Result<FileDownloadContext> prepareDownload(qint64 id, qint64 speedLimit, const QString& rangeHeader, QObject* streamParent) const;
        [[nodiscard]] Result<void> logDownloadRequest(qint64 id) const;
        [[nodiscard]] Result<JobStatus> getOrSubmitBatchJob(const QList<qint64>& ids);
        [[nodiscard]] Result<FileDownloadContext> getBatchFile(const QString& taskId, QObject* streamParent) const;

    private:
        std::shared_ptr<AudioRecordMapper> m_mapper;
        std::unique_ptr<FileIndexer> m_fileIndexer;
        DatabaseConfig m_dbConfig;
        NetworkConfig m_netConfig;
        mutable QReadWriteLock m_jobsLock;
        QHash<QString, JobStatus> m_jobs;
        void garbageCollectJobs();    // 清理垃圾任务和文件
    };
}

#endif