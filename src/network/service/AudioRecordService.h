#ifndef AUDIO_RECORD_SERVICE_H
#define AUDIO_RECORD_SERVICE_H

#include <QObject>
#include <memory>
#include <vector>
#include <QCryptographicHash>
#include "../utils/FileIndexer.h"
#include "../NetworkDTO.h"
#include "./mapper/AudioRecordMapper.h"

namespace radar::network {
    class AudioRecordService : public QObject {
        Q_OBJECT

    public:
        explicit AudioRecordService(QObject *parent = nullptr);
        ~AudioRecordService() override = default;
        [[nodiscard]] Result<void> init(const DatabaseConfig& dbConfig, const NetworkConfig& netConfig);
        void start() const;

        static void stop();
        [[nodiscard]] Result<qint64> verifyToken(const QString& rawToken) const;
        [[nodiscard]] Result<int> getTotalCount(const QDateTime& startTime, const QDateTime& endTime) const;
        [[nodiscard]] Result<std::vector<AudioRecordDTO>> getRecordPage(const QDateTime& startTime, const QDateTime& endTime, int limit, int offset) const;
        [[nodiscard]] Result<FileDownloadContext> prepareDownload(qint64 id, qint64 speedLimit, const QString& rangeHeader, QObject* streamParent) const;
        [[nodiscard]] Result<void> logDownloadRequest(qint64 id) const;

    private:
        std::shared_ptr<AudioRecordMapper> m_mapper;    // 数据访问对象
        std::unique_ptr<FileIndexer> m_fileIndexer;
        DatabaseConfig m_dbConfig;
        NetworkConfig m_netConfig;
    };
}

#endif