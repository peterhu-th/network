#ifndef FILE_INDEXER_H
#define FILE_INDEXER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include "../core/Types.h"
#include "mapper/AudioRecordMapper.h"

namespace radar::network {
    class FileIndexer : public QObject {
        Q_OBJECT
    public:
        explicit FileIndexer(AudioRecordMapper* dbManager, QObject* parent = nullptr);
        Result<void> start(const QString& rootPath, int intervalMs = 600000);    // 每 10 分钟查询一次
        [[nodiscard]] Result<void> scan();
    private:
        AudioRecordMapper* m_dbManager;
        QString m_rootPath;
        QTimer* m_timer;
        [[nodiscard]] Result<void> scanDirectory(const QString& path);
        [[nodiscard]] Result<void> processFile(const QString& filePath) const;
        static AudioRecord parseMetadata(const QString& audioPath);
        static QDateTime getGenerationTime(const QString& jsonPath, const QString& audioPath);
    };
}
#endif
