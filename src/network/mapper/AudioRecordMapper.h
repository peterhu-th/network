#ifndef AUDIO_RECORD_MAPPER_H
#define AUDIO_RECORD_MAPPER_H

#include <QObject>
#include <vector>
#include <map>
#include <mutex>
#include "../core/Types.h"
#include "../NetworkDefines.h"

// 负责音频记录数据的查询（当前为联调使用内存Mock模式，避免物理数据库依赖）
namespace radar::network {
    class AudioRecordMapper : public QObject {
        Q_OBJECT
    public:
        explicit AudioRecordMapper(const QString& connectionName, QObject* parent = nullptr);
        ~AudioRecordMapper() override;
        
        Result<void> insertRecord(const AudioRecord& record);                           // 插入记录
        Result<AudioRecord> getRecord(int64_t id);                                      // 获取单条记录
        Result<QString> getFilePathById(qint64 id);                                     // 获取本地文件路径
        Result<std::vector<AudioRecord>> queryRecords(const QDateTime& startTime, const QDateTime& endTime,
            int limit = 100, int offset = 0);                                           // 分页查询 API
        Result<int> countRecords(const QDateTime& startTime, const QDateTime& endTime); // 计数统计（用于计算分页总数）
        Result<bool> hasRecord(const QString& filePath);                                // 检查是否存在该路径记录
    private:
        QString m_connectionName;
        std::mutex m_mutex;
        
        std::map<int64_t, AudioRecord> m_records_map; // ID 到记录的映射
        std::vector<AudioRecord> m_records_list;      // 记录列表，用于排序和分页展示
    };
}

#endif
