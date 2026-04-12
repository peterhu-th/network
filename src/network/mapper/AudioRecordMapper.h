#ifndef AUDIO_RECORD_MAPPER_H
#define AUDIO_RECORD_MAPPER_H

#include <vector>
#include "../core/Types.h"
#include "NetworkDTO.h"
/*
    用到的 QSql 函数：
        exec(): bool 类型的执行函数
        next(): bool 类型的下移函数
 */
namespace radar::network {
    class AudioRecordMapper : public QObject {
        Q_OBJECT

    public:
        explicit AudioRecordMapper(QString connectionName, QObject* parent = nullptr);
        ~AudioRecordMapper() override = default;

        [[nodiscard]] Result<std::vector<std::pair<qint64, QString>>> getAllFilePaths() const;                  // 获取所有记录的 ID 和路径
        [[nodiscard]] Result<void> insertRecord(const AudioRecord& record) const;                               // 插入记录
        [[nodiscard]] Result<void> deleteRecord(qint64 id) const;                                               // 删除记录
        [[nodiscard]] Result<QString> getFilePathById(qint64 id) const;                                         // 获取本地文件路径
        [[nodiscard]] Result<std::vector<AudioRecord>> queryRecords(const QDateTime& startTime, const QDateTime& endTime,
            int limit = 100, int offset = 0) const;                                                             // 分页查询
        [[nodiscard]] Result<int> countRecords(const QDateTime& startTime, const QDateTime& endTime) const;     // 数据计数
        [[nodiscard]] Result<bool> hasRecord(const QString& filePath) const;                                    // 记录查重
        [[nodiscard]] Result<void> insertDownloadLog(qint64 fileId, const QDateTime& time) const;               // 记录日志

    private:
        QString m_connectionName;
        [[nodiscard]] Result<void> createTable() const;
    };
}

#endif
