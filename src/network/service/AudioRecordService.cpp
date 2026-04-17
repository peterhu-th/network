#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QFileInfo>
#include <QMimeDatabase>
#include "AudioRecordService.h"
#include "../mapper/AudioRecordMapper.h"
#include "../utils/ThrottledFile.h"
#include "../utils/ZipUtils.h"
#include <QDir>
#include <QtConcurrent>

namespace radar::network {
    AudioRecordService::AudioRecordService(QObject *parent): QObject(parent) {}

    Result<void> AudioRecordService::init(const DatabaseConfig& dbConfig, const NetworkConfig& netConfig) {
        m_dbConfig = dbConfig;
        m_netConfig = netConfig;
        QSqlDatabase db = QSqlDatabase::addDatabase(m_dbConfig.type, m_netConfig.globalConnectionName);
        db.setHostName(dbConfig.host);
        db.setPort(dbConfig.port);
        db.setDatabaseName(dbConfig.dbName);
        db.setUserName(dbConfig.username);
        db.setPassword(dbConfig.passWord);
        db.setConnectOptions("connect_timeout=1");
        if (!db.open()) {
            return Result<void>::error("Service database init failed: " + db.lastError().text(), ErrorCode::DatabaseInitFailed);
        }
        m_mapper = std::make_shared<AudioRecordMapper>(m_netConfig.globalConnectionName);
        m_fileIndexer = std::make_unique<FileIndexer>(m_mapper.get(), this);
        return Result<void>::ok();
    }

    void AudioRecordService::start() const {
        if (m_fileIndexer) {
            auto res = m_fileIndexer->start(m_dbConfig.storagePath);
            if (!res.isOk()) {
                qWarning() << "FileIndexer start failed: " << res.errorMessage();
            }
        }
    }
    
    Result<void> AudioRecordService::forceScan() const {
        if (m_fileIndexer) {
            auto res = m_fileIndexer->scan();
            if (!res.isOk()) {
                return Result<void>::error(res.errorMessage(), res.errorCode());
            }
        }
        return Result<void>::ok();
    }

    void AudioRecordService::stop() {}

    Result<int> AudioRecordService::getTotalCount(const QDateTime &startTime, const QDateTime &endTime, const QString& format) const {
        return m_mapper->countRecords(startTime, endTime, format);
    }

    Result<std::vector<AudioRecordDTO>> AudioRecordService::getRecordPage(const QDateTime& startTime, const QDateTime& endTime, const QString& format, int limit, int offset) const {
        auto dbResult = m_mapper->queryRecords(startTime, endTime, format, limit, offset);
        if (!dbResult.isOk()) {
            return Result<std::vector<AudioRecordDTO>>::error(dbResult.errorMessage(), dbResult.errorCode());
        }
        // 将实体转换为 DTO
        std::vector<AudioRecordDTO> dtoList;
        for (const auto& record : dbResult.value()) {
            AudioRecordDTO dto;
            dto.id = record.id;
            dto.duration = record.duration;
            dto.fileSize = record.fileSize;
            dto.generationTime = record.generationTime;
            dtoList.push_back(dto);
        }
        return Result<std::vector<AudioRecordDTO>>::ok(dtoList);
    }

    Result<FileDownloadContext> AudioRecordService::prepareDownload(qint64 id, qint64 speedLimit, const QString& rangeHeader, QObject* streamParent) const {
        // 获取物理路径
        auto pathRes = m_mapper->getFilePathById(id);
        if (!pathRes.isOk()) return Result<FileDownloadContext>::error("Record not found", ErrorCode::RecordNotFound);
        const QString& filePath = pathRes.value();
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists()) return Result<FileDownloadContext>::error("File missing", ErrorCode::FileNotExist);
        // 准备上下文元数据
        FileDownloadContext context;
        context.fileName = fileInfo.fileName();
        context.fileSize = fileInfo.size();
        context.endPos = context.fileSize - 1;
        context.contentType = QMimeDatabase().mimeTypeForFile(filePath).name();
        // 解析 Range 头实现断点续传
        if (!rangeHeader.isEmpty() && rangeHeader.startsWith("bytes=")) {
            QStringList parts = rangeHeader.mid(6).split("-");
            if (!parts[0].isEmpty()) context.startPos = parts[0].toLongLong();
            if (parts.size() > 1 && !parts[1].isEmpty()) context.endPos = parts[1].toLongLong();
            if (context.startPos >= context.fileSize) return Result<FileDownloadContext>::error("Invalid Range", ErrorCode::InvalidParam);
            context.isPartial = true;
        }
        // 构建限速流
        auto file = std::make_unique<ThrottledFile>(filePath, speedLimit, streamParent);
        if (!file->open(QIODevice::ReadOnly)) {
            return Result<FileDownloadContext>::error("IO Error", ErrorCode::NetworkFileIOFailed);
        }
        file->seek(context.startPos);
        context.stream = file.release();
        // 记录日志
        auto logRes = logDownloadRequest(id);
        if (!logRes.isOk()) {
            return Result<FileDownloadContext>::error(logRes.errorMessage(), logRes.errorCode());
        }
        return Result<FileDownloadContext>::ok(context);
    }

    Result<void> AudioRecordService::logDownloadRequest(qint64 id) const {
        auto res = m_mapper->insertDownloadLog(id, QDateTime::currentDateTime());
        if (!res.isOk()) {
            qWarning() << "DB Log failed for ID:" << id << "-" << res.errorMessage();
            return Result<void>::error("Insert download log failed", ErrorCode::NetworkFileIOFailed);
        }
        qInfo() << "Download successfully initiated for file ID:" << id;
        return Result<void>::ok();
    }

    void AudioRecordService::garbageCollectJobs() {
        QWriteLocker locker(&m_jobsLock);
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        auto it = m_jobs.begin();
        while (it != m_jobs.end()) {
            if (now - it.value().createdAt > 3600000) {
                if (!it.value().resultPath.isEmpty()) {
                    QFile::remove(it.value().resultPath);
                }
                it = m_jobs.erase(it);
            } else {
                ++it;
            }
        }
    }

    Result<JobStatus> AudioRecordService::getOrSubmitBatchJob(const QList<qint64>& ids) {
        garbageCollectJobs();
        if (ids.isEmpty()) return Result<JobStatus>::error("Empty tasks", ErrorCode::InvalidParam);
        // 利用 ids 排序构建唯一的固定 Hash
        QList<qint64> sortedIds = ids;
        std::sort(sortedIds.begin(), sortedIds.end());
        QByteArray idBytes;
        for (qint64 id : sortedIds) idBytes.append(QString::number(id).toUtf8() + ",");
        QByteArray hashBytes = QCryptographicHash::hash(idBytes, QCryptographicHash::Sha256);
        QString taskId = QString(hashBytes.toHex());
        
        // 任务已存在，提取进度返回
        {
            QReadLocker locker(&m_jobsLock);
            if (m_jobs.contains(taskId)) {
                return Result<JobStatus>::ok(m_jobs[taskId]);
            }
        }

        JobStatus newJob;
        newJob.taskId = taskId;
        newJob.status = "Processing";
        newJob.createdAt = QDateTime::currentMSecsSinceEpoch();
        newJob.resultPath = QDir::tempPath() + "/batch_records_" + taskId + ".zip";

        {
            QWriteLocker locker(&m_jobsLock);
            m_jobs[taskId] = newJob;
        }

        QList<QString> filePaths;
        for (qint64 id : sortedIds) {
            auto pathRes = m_mapper->getFilePathById(id);
            if (pathRes.isOk()) {
                filePaths.append(pathRes.value());
            }
        }

        if (filePaths.isEmpty()) {
            QWriteLocker locker(&m_jobsLock);
            m_jobs[taskId].status = "Failed";
            return Result<JobStatus>::error("Empty valid files", ErrorCode::RecordNotFound);
        }
        // 异步并发
        (void) QtConcurrent::run([this, taskId, filePaths, resultPath = newJob.resultPath]() {
            auto res = ZipUtils::createZip(filePaths, resultPath);
            QWriteLocker locker(&m_jobsLock);
            if (m_jobs.contains(taskId)) {
                m_jobs[taskId].status = res.isOk() ? "Completed" : "Failed";
            }
        });
        // 返回状态为 Processing
        return Result<JobStatus>::ok(newJob);
    }

    Result<FileDownloadContext> AudioRecordService::getBatchFile(const QString& taskId, QObject* streamParent) const {
        QString resultPath;

        {
            QReadLocker locker(&m_jobsLock);
            if (!m_jobs.contains(taskId)) {
                return Result<FileDownloadContext>::error("Task not found", ErrorCode::RecordNotFound);
            }
            if (m_jobs[taskId].status != "Completed") {
                return Result<FileDownloadContext>::error("Task is still processing", ErrorCode::TaskProcessingFailed);
            }
            resultPath = m_jobs[taskId].resultPath;
        }

        QFileInfo fileInfo(resultPath);
        if (!fileInfo.exists()) {
            return Result<FileDownloadContext>::error("Zip file missing on disk", ErrorCode::FileNotExist);
        }
        FileDownloadContext ctx;
        ctx.fileName = "batch_records_" + taskId.left(6) + ".zip";
        ctx.contentType = "application/zip";
        ctx.fileSize = fileInfo.size();
        ctx.endPos = ctx.fileSize - 1;
        auto file = std::make_unique<ThrottledFile>(resultPath, 0, streamParent); 
        if (!file->open(QIODevice::ReadOnly)) {
            return Result<FileDownloadContext>::error("File unreadable", ErrorCode::NetworkFileIOFailed);
        }
        ctx.stream = file.release();
        return Result<FileDownloadContext>::ok(ctx);
    }
}