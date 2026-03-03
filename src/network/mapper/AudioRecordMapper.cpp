#include "AudioRecordMapper.h"

namespace radar::network {
    AudioRecordMapper::AudioRecordMapper(const QString& connectionName, QObject* parent)
            : QObject(parent), m_connectionName(connectionName) {
        // Mock 模式：当前仅为联调使用，暂不实际发起网络与 QSqlDatabase 连接以避免由于数据库未启动等问题抛抛异常阻断环境，数据来源于 audio_test 的本地文件扫描。
    }

    AudioRecordMapper::~AudioRecordMapper() {
        // 清理 Mock 存储
        m_records_map.clear();
        m_records_list.clear();
    }

    Result<void> AudioRecordMapper::insertRecord(const AudioRecord &record) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_records_map[record.id] = record;
        m_records_list.push_back(record);
        
        // 维持倒序排列 (ORDER BY generation_time DESC)
        std::sort(m_records_list.begin(), m_records_list.end(), [](const AudioRecord& a, const AudioRecord& b) {
            return a.generationTime > b.generationTime;
        });
        
        return Result<void>::ok();
    }

    Result<AudioRecord> AudioRecordMapper::getRecord(int64_t id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_records_map.find(id);
        if (it != m_records_map.end()) {
            return Result<AudioRecord>::ok(it->second);
        }
        return Result<AudioRecord>::error("Record not found (Mock)", ErrorCode::RecordNotFound);
    }

    Result<QString> AudioRecordMapper::getFilePathById(qint64 id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_records_map.find(id);
        if (it != m_records_map.end()) {
            return Result<QString>::ok(it->second.filePath);
        }
        return Result<QString>::error("Failed to get file path (Mock)", ErrorCode::DatabaseQueryFailed);
    }

    Result<std::vector<AudioRecord> > AudioRecordMapper::queryRecords(const QDateTime &startTime, const QDateTime &endTime, int limit, int offset) {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<AudioRecord> results;
        
        // 执行记录条件过滤
        for (const auto& record : m_records_list) {
            if (startTime.isValid() && record.generationTime < startTime) continue;
            if (endTime.isValid() && record.generationTime > endTime) continue;
            results.push_back(record);
        }
        
        // 执行分页切割 (Pagination)
        if (offset >= results.size()) {
            return Result<std::vector<AudioRecord>>::ok(std::vector<AudioRecord>()); // 偏移超过总数，返回空数组
        }
        
        int endIdx = std::min(static_cast<int>(results.size()), offset + limit);
        std::vector<AudioRecord> pageResults(results.begin() + offset, results.begin() + endIdx);
        
        return Result<std::vector<AudioRecord>>::ok(pageResults);
    }

    Result<int> AudioRecordMapper::countRecords(const QDateTime &startTime, const QDateTime &endTime) {
        std::lock_guard<std::mutex> lock(m_mutex);
        int count = 0;
        for (const auto& record : m_records_list) {
            if (startTime.isValid() && record.generationTime < startTime) continue;
            if (endTime.isValid() && record.generationTime > endTime) continue;
            count++;
        }
        return Result<int>::ok(count);
    }

    Result<bool> AudioRecordMapper::hasRecord(const QString &filePath) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& record : m_records_list) {
            if (record.filePath == filePath) {
                return Result<bool>::ok(true);
            }
        }
        return Result<bool>::ok(false);
    }
}