#include "FileIndexer.h"
#include "IdGenerator.h"
#include <QDirIterator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QDebug>

namespace radar::network {
    FileIndexer::FileIndexer(DatabaseManager* dbManager, QObject* parent)
        : QObject(parent), m_dbManager(dbManager) {
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &FileIndexer::scan);
    }

    Result<void> FileIndexer::start(const QString& rootPath, int intervalMs) {
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
        qDebug() << "Starting file scan in" << m_rootPath;
        if (m_rootPath.isEmpty() || !QDir(m_rootPath).exists()) {
            qWarning() << "Storage path does not exist:" << m_rootPath;
            return Result<void>::error("Storage path does not exist", ErrorCode::InvalidConfig);
        }
        return scanDirectory(m_rootPath);
    }

    Result<void> FileIndexer::scanDirectory(const QString& path) const{
        QDirIterator it(path, QStringList() << "*.wav", QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            auto res = processFile(it.next());
            if (!res.isOk()) {
                return Result<void>::error("Failed to process file: " + res.errorMessage(), res.errorCode());
            }
        }
        return Result<void>::ok();
    }

    Result<void> FileIndexer::processFile(const QString& filePath) const{
        // 检查是否已存在
        auto hasRes = m_dbManager->hasRecord(filePath);
        if (!hasRes.isOk()) {
            return Result<void>::error(hasRes.errorMessage(), hasRes.errorCode());
        }
        if (hasRes.value()) {
            return Result<void>::ok();
        }

        qDebug() << "Indexing new file:" << filePath;
        AudioRecord record = parseMetadata(filePath);

        // 生成 ID
        auto idRes = IdGenerator::instance().nextId();
        if (!idRes.isOk()) {
            qWarning() << "Failed to generate ID:" << idRes.errorMessage();
            return Result<void>::error(idRes.errorMessage(), idRes.errorCode());
        }
        record.id = idRes.value();

        if (auto res = m_dbManager->insertRecord(record); !res.isOk()) {
            qWarning() << "Failed to insert record:" << res.errorMessage();
            return res;
        }
        return Result<void>::ok();
    }

    // 待 storage 模块完成后修改此处
    AudioRecord FileIndexer::parseMetadata(const QString& wavPath) {
        AudioRecord record;
        record.filePath = wavPath;
        QFileInfo wavInfo(wavPath);
        record.fileSize = wavInfo.size();
        record.durationMs = 0; // 需要读取 wav 头获取时长，这里暂时略过

        // 尝试寻找同名 json
        QString jsonPath = wavPath;
        jsonPath.replace(".wav", ".json");

        record.generationTime = getGenerationTime(jsonPath, wavPath);
        record.createdAt = QDateTime::currentDateTime();

        return record;
    }

    QDateTime FileIndexer::getGenerationTime(const QString& jsonPath, const QString& wavPath) {
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
        return QFileInfo(wavPath).lastModified();
    }
}
