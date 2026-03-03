#ifndef AUDIO_RECORD_MAPPER_H
#define AUDIO_RECORD_MAPPER_H

#include <QSqlDatabase>
#include <vector>
#include <mutex>

#include "../core/types.h"
#include "../NetworkDefines.h"

namespace radar::network {
    // 与 SQL 数据库交互
    class AudioRecordMapper : public QObject {
        Q_OBJECT
    public:
        AudioRecordMapper();
        ~AudioRecordMapper() override;
        Result<void> init(const DatabaseConfig& config);
        Result<void> insertRecord(const AudioRecord& record);                           // 插入记录
        Result<AudioRecord> getRecord(int64_t id);                                      // 获取单条记录
        Result<QString> getFilePathById(qint64 id);                                     // 获取本地文件路径
        Result<std::vector<AudioRecord>> queryRecords(const QDateTime& startTime, const QDateTime& endTime,
            int limit = 100, int offset = 0);                                           // 分页查询
        Result<int> countRecords(const QDateTime& startTime, const QDateTime& endTime); // 计数
        Result<bool> hasRecord(const QString& filePath);
    private:
        QSqlDatabase m_db;
        std::mutex m_mutex;
        QString m_connectionName;
        [[nodiscard]] Result<void> createTable() const;
    };
}

#endif
