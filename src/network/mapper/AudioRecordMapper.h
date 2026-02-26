#ifndef AUDIO_RECORD_MAPPER_H
#define AUDIO_RECORD_MAPPER_H

#include "../NetworkDefines.h"
#include "../../core/Types.h"
#include <QObject>
#include <QSqlDatabase>
#include <vector>
#include <mutex>

namespace radar::network::mapper {
    class AudioRecordMapper : public QObject {
        Q_OBJECT
    public:
        explicit AudioRecordMapper(QObject* parent = nullptr);
        ~AudioRecordMapper() override;

        Result<void> init(const DatabaseConfig& config);
        Result<void> insertRecord(const AudioRecord& record);
        Result<AudioRecord> getRecord(int64_t id);
        
        // 分页查询：增加 offset 和 limit 参数
        Result<std::vector<AudioRecord>> queryRecords(const QDateTime& startTime, const QDateTime& endTime, int limit = 100, int offset = 0);
        
        // 查询总记录数，用于分页
        Result<int> countRecords(const QDateTime& startTime, const QDateTime& endTime);
        
        Result<bool> hasRecord(const QString& filePath);

    private:
        QSqlDatabase m_db;
        std::mutex m_mutex;
        QString m_connectionName;

        [[nodiscard]] Result<void> createTable() const;
    };
}

#endif // AUDIO_RECORD_MAPPER_H
