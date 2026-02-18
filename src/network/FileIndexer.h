#ifndef FILE_INDEXER_H
#define FILE_INDEXER_H

#include "DatabaseManager.h"
#include <QObject>
#include <QString>
#include <QTimer>

namespace radar {
namespace network {

/**
 * @brief 文件索引器
 * @details 扫描指定目录下的 .wav 文件，并将元数据存入数据库。
 */
class FileIndexer : public QObject {
    Q_OBJECT
public:
    explicit FileIndexer(DatabaseManager* dbManager, QObject* parent = nullptr);

    /**
     * @brief 启动索引服务
     * @param rootPath 根目录路径
     * @param intervalMs 扫描间隔（毫秒），0表示仅启动时扫描一次
     */
    void start(const QString& rootPath, int intervalMs = 60000);

    /**
     * @brief 手动执行一次扫描
     */
    void scan();

private:
    DatabaseManager* m_dbManager;
    QString m_rootPath;
    QTimer* m_timer;

    void scanDirectory(const QString& path);
    void processFile(const QString& filePath);
    AudioRecord parseMetadata(const QString& wavPath);
    QDateTime getGenerationTime(const QString& jsonPath, const QString& wavPath);
};

} // namespace network
} // namespace radar

#endif // FILE_INDEXER_H
