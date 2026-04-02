#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QFileInfo>
#include <QMimeDatabase>
#include "AudioRecordService.h"
#include "./mapper/AudioRecordMapper.h"
#include "../utils/ThrottledFile.h"

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
        if (!db.open()) {
            return Result<void>::error("Service database init failed: " + db.lastError().text(), ErrorCode::DatabaseInitFailed);
        }
        m_mapper = std::make_shared<AudioRecordMapper>(m_netConfig.globalConnectionName);
        m_fileIndexer = std::make_unique<FileIndexer>(m_mapper.get(), m_dbConfig.ffprobePath, this);
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

    void AudioRecordService::stop() {}

    Result<qint64> AudioRecordService::verifyToken(const QString& rawToken) const{
        QString token = rawToken;
        if (token.startsWith("Bearer ", Qt::CaseInsensitive)) {
            token = token.mid(7).trimmed();
        }
        QStringList parts = token.split("_");
        if (parts.size() != 3) {
            return Result<qint64>::error("Invalid Token Format", ErrorCode::AuthorizationFailed);
        }
        qint64 uid = parts[0].toLongLong();
        qint64 timestamp = parts[1].toLongLong();
        const QString& sign = parts[2];
        // 时效校验
        qint64 currentTs = QDateTime::currentSecsSinceEpoch();
        if (currentTs - timestamp > 7200 || currentTs - timestamp < 0) {
            return Result<qint64>::error("Token Expired", ErrorCode::AuthorizationFailed);
        }
        // 验签: Hash(UID + "_" + TIME + "_" + SECRET)
        QString dataToHash = parts[0] + "_" + parts[1] + "_" + m_netConfig.serverSecret;
        QString expectedSign = QString(QCryptographicHash::hash(dataToHash.toUtf8(), QCryptographicHash::Sha256).toHex());

        if (sign != expectedSign) {
            return Result<qint64>::error("Signature Mismatch", ErrorCode::AuthorizationFailed);
        }
        // 返回 UID
        return Result<qint64>::ok(uid);
    }

    Result<int> AudioRecordService::getTotalCount(const QDateTime &startTime, const QDateTime &endTime) const {
        return m_mapper->countRecords(startTime, endTime);
    }

    Result<std::vector<AudioRecordDTO>> AudioRecordService::getRecordPage(const QDateTime& startTime, const QDateTime& endTime, int limit, int offset) const {
        auto dbResult = m_mapper->queryRecords(startTime, endTime, limit, offset);
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
}