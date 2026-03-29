#include <QDirIterator>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
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
        AudioRecord record = parseMetadata(filePath);
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

    AudioRecord FileIndexer::parseMetadata(const QString &audioPath) {
        AudioRecord record;
        record.filePath = audioPath;
        record.fileSize = QFileInfo(audioPath).size();
        record.duration = 0;    //TODO: 读取 wav 文件获取时长
        QFileInfo fileInfo(audioPath);
        QString jsonPath = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + ".json";
        record.generationTime = getGenerationTime(jsonPath, audioPath);
        record.createdAt = QDateTime::currentDateTime();
        return record;
    }
    // 若 json 文件中无信息则使用操作系统记录的最后修改时间
    QDateTime FileIndexer::getGenerationTime(const QString &jsonPath, const QString &audioPath) {
        QFile jsonFile(jsonPath);
        if (jsonFile.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll());
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject obj = doc.object();
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
}