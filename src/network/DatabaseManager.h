#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include "NetworkDefines.h"
#include "Types.h"
#include <QObject>
#include <QSqlDatabase>
#include <vector>
#include <mutex>

namespace radar::network {
    // 数据库管理器：负责 MySQL 数据库的连接、初始化和 CRUD 操作
    class DatabaseManager : public QObject {
        Q_OBJECT
    public:
        explicit DatabaseManager(QObject* parent = nullptr);
        ~DatabaseManager() override;

        //初始化数据库链接
        Result<void> init(const DatabaseConfig& config);
        // 插入音频记录
        Result<void> insertRecord(const AudioRecord& record);
        // 根据 ID 获取记录
        Result<AudioRecord> getRecord(int64_t id);
        // 查询一定数量的记录列表
        Result<std::vector<AudioRecord>> queryRecords(const QDateTime& startTime, const QDateTime& endTime, int limit = 100);
        //查重
        Result<bool> hasRecord(const QString& filePath);

    private:
        QSqlDatabase m_db;
        std::mutex m_mutex;
        QString m_connectionName;

        [[nodiscard]] Result<void> createTable() const;
    };
}

#endif // DATABASE_MANAGER_H
