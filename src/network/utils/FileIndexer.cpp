#include <QDirIterator>
#include <QFileInfo>
#include <QProcess>
#include <QCoreApplication>
#include <QDir>
#include <taglib/fileref.h>
#include <taglib/audioproperties.h>
#include "IdGenerator.h"
#include "FileIndexer.h"

namespace radar::network {
    FileIndexer::FileIndexer(AudioRecordMapper *dbManager, QObject *parent)
        : QObject(parent), m_dbManager(dbManager) {
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

    Result<void> FileIndexer::scanDirectory(const QString &path) const {
        // 获取数据库现有记录
        auto allDbRes = m_dbManager->getAllFilePaths();
        if (!allDbRes.isOk()) {
            return Result<void>::error("Failed to fetch DB records: " + allDbRes.errorMessage(), allDbRes.errorCode());
        }

        // 将数据库记录存入哈希表
        std::unordered_map<QString, qint64> dbRecordsMap;
        for (const auto& pair : allDbRes.value()) {
            dbRecordsMap[pair.second] = pair.first;
        }

        // 遍历本地目录
        QDirIterator it(path, QStringList() << "*.wav" << "*.mp3" << "*.m4a", QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString filePath = it.next();

            auto mapIt = dbRecordsMap.find(filePath);
            if (mapIt != dbRecordsMap.end()) {
                dbRecordsMap.erase(mapIt);
            } else {
                auto res = processFile(filePath);
                if (!res.isOk()) {
                    qWarning() << "Failed to process new file: " << filePath << ", error: " << res.errorMessage();
                } else {
                    qInfo() << "Added new file to database: " << filePath;
                }
            }
        }
        // 清理残留数据
        for (const auto& pair : dbRecordsMap) {
            auto deleteRes = m_dbManager->deleteRecord(pair.second);
            if (!deleteRes.isOk()) {
                qWarning() << "Failed to delete missing file record ID: " << pair.second
                           << ", error: " << deleteRes.errorMessage();
            } else {
                qInfo() << "Deleted DB record for missing local file: " << pair.first;
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

    Result<AudioRecord> FileIndexer::parseMetadata(const QString &audioPath) {
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

    Result<int> FileIndexer::getAudioDuration(const QString &filePath) {
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists()) {
            return Result<int>::error("File not found: " + filePath, ErrorCode::InvalidParam);
        }
        TagLib::FileRef f(reinterpret_cast<const wchar_t*>(filePath.utf16()));
        // 解析并获取时长
        if (!f.isNull() && f.audioProperties()) {
            int durationSeconds = f.audioProperties()->lengthInSeconds();
            if (durationSeconds >= 0) {
                return Result<int>::ok(durationSeconds);
            }
        }
        return Result<int>::error("Failed to parse duration natively for: " + filePath, ErrorCode::ToolsError);
    }
}