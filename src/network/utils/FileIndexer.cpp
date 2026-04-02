#include <QDirIterator>
#include <QFileInfo>
#include <QProcess>
#include <QCoreApplication>
#include <QDir>
#include "IdGenerator.h"
#include "FileIndexer.h"

namespace radar::network {
    FileIndexer::FileIndexer(AudioRecordMapper *dbManager, QString ffprobePath, QObject *parent)
        : QObject(parent), m_dbManager(dbManager), m_ffprobePath(std::move(ffprobePath)) {
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, [this]() {
            auto res = this->scan();
            if (!res.isOk()) {
                qWarning() << "Auto scan failed:" << res.errorMessage();
            }
            return Result<void>::ok();
        });
    }

    Result<void> FileIndexer::start(const QString &rootPath, int intervalMs) {
        m_rootPath = rootPath;
        auto res = scan();
        if (!res.isOk()) {
            return res;
        }
        if (intervalMs > 0) {
            m_timer->start(intervalMs);
        }
        return Result<void>::ok();
    }

    Result<void> FileIndexer::scan() const{
        if (m_rootPath.isEmpty()) {
            return Result<void>::error("Path does not exist", ErrorCode::InvalidConfig);
        }
        return scanDirectory(m_rootPath);
    }

    Result<void> FileIndexer::scanDirectory(const QString &path) const{
        // 递归搜索子目录
        QDirIterator it(path, QStringList() << "*.wav" << "*.mp3" << "*.m4a", QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString filePath = it.next();
            auto res = processFile(filePath);
            if (!res.isOk()) {
                qWarning() << "Failed to process file: " << filePath << ", error: " << res.errorMessage();
            }
        }
        return Result<void>::ok();
    }

    Result<void> FileIndexer::processFile(const QString &filePath) const {
        // 查重
        auto hasRes = m_dbManager->hasRecord(filePath);
        if (!hasRes.isOk()) {
            return Result<void>::error(hasRes.errorMessage(), hasRes.errorCode());
        }
        if (hasRes.value()) {
            return Result<void>::ok();
        }
        // 提取元数据
        auto metaRes = parseMetadata(filePath);
        if (!metaRes.isOk()) {
            return Result<void>::error(metaRes.errorMessage(), metaRes.errorCode());
        }
        AudioRecord record = metaRes.value();
        // 分配 ID
        auto idRes = IdGenerator::instance().nextId();
        if (!idRes.isOk()) {
            return Result<void>::error(idRes.errorMessage(), idRes.errorCode());
        }
        record.id = idRes.value();
        // 写入数据库
        if (auto res = m_dbManager->insertRecord(record); !res.isOk()) {
            return res;
        }
        return Result<void>::ok();
    }

    Result<AudioRecord> FileIndexer::parseMetadata(const QString &audioPath) const {
        AudioRecord record;
        record.filePath = audioPath;
        record.fileSize = QFileInfo(audioPath).size();
        auto durationRes = getAudioDuration(audioPath);
        if (!durationRes.isOk()) {
            return Result<AudioRecord>::error(durationRes.errorMessage(), durationRes.errorCode());
        }
        record.duration = durationRes.value();
        QFileInfo fileInfo(audioPath);
        QString jsonPath = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + ".json";
        record.generationTime = getGenerationTime(jsonPath, audioPath);
        record.createdAt = QDateTime::currentDateTime();
        return Result<AudioRecord>::ok(record);
    }

    QDateTime FileIndexer::getGenerationTime(const QString &jsonPath, const QString &audioPath) {
        QFile jsonFile(jsonPath);
        if (jsonFile.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll());
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject obj = doc.object();
                // 若 json 文件中无信息则使用操作系统记录的最后修改时间
                if (obj.contains("timestamp")) {
                    int64_t ts = obj["timestamp"].toVariant().toLongLong();
                    return QDateTime::fromMSecsSinceEpoch(ts);
                }
                if (obj.contains("time")) {
                    return QDateTime::fromString(obj["time"].toString(), Qt::ISODate);
                }
            }
        }
        return QFileInfo(audioPath).lastModified();
    }

    Result<int> FileIndexer::getAudioDuration(const QString &filePath) const{
        QString ffprobePath = m_ffprobePath;
        QFileInfo toolInfo(ffprobePath);
        if (!toolInfo.exists()) {
            return Result<int>::error("ffprobe not found by path: " + ffprobePath, ErrorCode::ToolsError);
        }
        QProcess process;
        QStringList args;
        args << "-v" << "error"
             << "-show_entries" << "format=duration"
             << "-of" << "default=noprint_wrappers=1:nokey=1"
             << filePath;
        process.start(ffprobePath, args);

        if (!process.waitForFinished(2000)) {
            process.kill();
            return Result<int>::error("ffprobe timed out: " + filePath, ErrorCode::ToolsError);
        }
        QString output = process.readAllStandardOutput().trimmed();
        bool ok;
        double durationSeconds = output.toDouble(&ok);
        if (ok) {
            return Result<int>::ok(qRound(durationSeconds));
        }

        return Result<int>::error("invalid ffprobe output: " + output, ErrorCode::ToolsError);
    }
}