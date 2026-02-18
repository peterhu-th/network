#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include "NetworkDefines.h"
#include "Types.h" // for Result
#include <QObject>
#include <QSqlDatabase>
#include <vector>
#include <mutex>

namespace radar {
namespace network {

/**
 * @brief 数据库管理器
 * @details 负责 MySQL 数据库的连接、初始化和 CRUD 操作。
 */
class DatabaseManager : public QObject {
    Q_OBJECT
public:
    explicit DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager();

    /**
     * @brief 初始化数据库连接
     * @param config 数据库配置
     * @return Result<void>
     */
    Result<void> init(const DatabaseConfig& config);

    /**
     * @brief 插入音频记录
     * @param record 音频记录
     * @return Result<void>
     */
    Result<void> insertRecord(const AudioRecord& record);

    /**
     * @brief 根据 ID 获取记录
     * @param id 记录 ID
     * @return Result<AudioRecord>
     */
    Result<AudioRecord> getRecord(int64_t id);

    /**
     * @brief 查询记录列表
     * @param startTime 开始时间
     * @param endTime 结束时间
     * @param limit 限制数量
     * @return Result<std::vector<AudioRecord>>
     */
    Result<std::vector<AudioRecord>> queryRecords(const QDateTime& startTime, const QDateTime& endTime, int limit = 100);

    /**
     * @brief 检查文件路径是否已存在（用于去重）
     * @param filePath 文件路径
     * @return bool
     */
    bool hasRecord(const QString& filePath);

private:
    QSqlDatabase m_db;
    std::mutex m_mutex;
    QString m_connectionName;

    bool createTable();
};

} // namespace network
} // namespace radar

#endif // DATABASE_MANAGER_H
