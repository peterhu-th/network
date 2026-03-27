#ifndef AUDIO_RECORD_MAPPER_H
#define AUDIO_RECORD_MAPPER_H

#include <vector>
#include "../core/Types.h"
#include "../NetworkDefines.h"

namespace radar::network {
    class AudioRecordMapper : public QObject {
        Q_OBJECT
    public:
        explicit AudioRecordMapper(QString connectionName, QObject* parent = nullptr);
        ~AudioRecordMapper() override = default;
        
        [[nodiscard]] Result<void> insertRecord(const AudioRecord& record) const;                             // 插入记录
        [[nodiscard]] Result<AudioRecord> getRecord(int64_t id) const;                                        // 获取单条记录
        [[nodiscard]] Result<QString> getFilePathById(qint64 id) const;                                       // 获取本地文件路径
        [[nodiscard]] Result<std::vector<AudioRecord>> queryRecords(const QDateTime& startTime, const QDateTime& endTime,
            int limit = 100, int offset = 0) const;                                                   // 分页查询 API
        [[nodiscard]] Result<int> countRecords(const QDateTime& startTime, const QDateTime& endTime) const;   // 计数统计（用于计算分页总数）
        [[nodiscard]] Result<bool> hasRecord(const QString& filePath) const;                                  // 检查是否存在该路径记录
    private:
        QString m_connectionName;
        [[nodiscard]] Result<void> createTable() const;
    };
}

#endif
