#ifndef FILE_INDEXER_H
#define FILE_INDEXER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include "../core/Types.h"
#include "mapper/AudioRecordMapper.h"
#include "../NetworkDTO.h"

namespace radar::network {
    class FileIndexer : public QObject {
        Q_OBJECT

    public:
        explicit FileIndexer(AudioRecordMapper* dbManager, QString ffprobePath, QObject* parent = nullptr);
        Result<void> start(const QString& rootPath, int intervalMs = 60000);    // 每 10 分钟查询一次
        [[nodiscard]] Result<void> scan() const;

    private:
        AudioRecordMapper* m_dbManager;
        QString m_rootPath;
        QString m_ffprobePath;
        QTimer* m_timer;
        [[nodiscard]] Result<void> scanDirectory(const QString& path) const;
        [[nodiscard]] Result<void> processFile(const QString& filePath) const;
        [[nodiscard]] Result<AudioRecord> parseMetadata(const QString &audioPath) const;
        static QDateTime getGenerationTime(const QString& jsonPath, const QString& audioPath);
        [[nodiscard]] Result<int> getAudioDuration(const QString &filePath) const;
    };
}
#endif
