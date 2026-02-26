#ifndef FILE_INDEXER_H
#define FILE_INDEXER_H

#include "../../core/Types.h"
#include "../mapper/AudioRecordMapper.h"
#include <QObject>
#include <QString>
#include <QTimer>

namespace radar::network {
    // 文件索引器：扫描指定目录下的 .wav 文件，并将元数据存入数据库
    class FileIndexer : public QObject {
        Q_OBJECT
    public:
        explicit FileIndexer(mapper::AudioRecordMapper* dbManager, QObject* parent = nullptr);
        // 启动自动扫描
        Result<void> start(const QString& rootPath, int intervalMs = 60000);
        // 手动扫描
        [[nodiscard]] Result<void> scan() const;

    private:
        mapper::AudioRecordMapper* m_dbManager;
        QString m_rootPath;
        QTimer* m_timer;

        [[nodiscard]] Result<void> scanDirectory(const QString& path) const;
        // 确认
        [[nodiscard]] Result<void> processFile(const QString& filePath) const;
        static AudioRecord parseMetadata(const QString& wavPath);
        static QDateTime getGenerationTime(const QString& jsonPath, const QString& wavPath);
    };
}

#endif // FILE_INDEXER_H
