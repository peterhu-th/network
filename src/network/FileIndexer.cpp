#include "FileIndexer.h"
#include "IdGenerator.h"
#include <QDir>
#include <QDirIterator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QDebug>

namespace radar {
namespace network {

FileIndexer::FileIndexer(DatabaseManager* dbManager, QObject* parent)
    : QObject(parent), m_dbManager(dbManager) {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &FileIndexer::scan);
}

void FileIndexer::start(const QString& rootPath, int intervalMs) {
    m_rootPath = rootPath;
    scan(); // 立即扫描一次
    if (intervalMs > 0) {
        m_timer->start(intervalMs);
    }
}

void FileIndexer::scan() {
    qDebug() << "Starting file scan in" << m_rootPath;
    if (m_rootPath.isEmpty() || !QDir(m_rootPath).exists()) {
        qWarning() << "Storage path does not exist:" << m_rootPath;
        return;
    }
    scanDirectory(m_rootPath);
}

void FileIndexer::scanDirectory(const QString& path) {
    QDirIterator it(path, QStringList() << "*.wav", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        processFile(it.next());
    }
}

void FileIndexer::processFile(const QString& filePath) {
    // 检查是否已存在
    if (m_dbManager->hasRecord(filePath)) {
        return;
    }

    qDebug() << "Indexing new file:" << filePath;
    AudioRecord record = parseMetadata(filePath);
    
    // 生成 ID
    record.id = IdGenerator::instance().nextId();
    
    if (auto res = m_dbManager->insertRecord(record); !res.isOk()) {
        qCritical() << "Failed to insert record:" << res.errorMessage();
    }
}

AudioRecord FileIndexer::parseMetadata(const QString& wavPath) {
    AudioRecord record;
    record.filePath = wavPath;
    QFileInfo wavInfo(wavPath);
    record.fileSize = wavInfo.size();
    record.durationMs = 0; // 需要读取 wav 头获取时长，这里暂时略过或模拟

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
                // 假设 timestamp 是微秒或毫秒，这里做简单判断
                // AudioFrame 使用微秒
                return QDateTime::fromMSecsSinceEpoch(ts / 1000); 
            }
            if (obj.contains("time")) {
                return QDateTime::fromString(obj["time"].toString(), Qt::ISODate);
            }
        }
    }

    // fallback: use file modification time
    return QFileInfo(wavPath).lastModified();
}

} // namespace network
} // namespace radar
