#ifndef TYPES_H
#define TYPES_H

#include <QString>
#include <QByteArray>
#include <QVariantMap>
#include <cstdint>

namespace radar {

struct UserConfig {
    qint64 id;
    QString username;
    QString password;
    QString passwordHash;
};

struct AudioFrame {
    int64_t timestamp = 0;
    int sampleRate = 44100;
    int channels = 1;
    int sampleSize = 16;
    QByteArray data;
    QString sourceId;
};

struct ProcessedData {
    AudioFrame originalFrame;
    bool isValid = false;
    double signalStrength = 0.0;
    QVariantMap features;
};

struct SourceConfig {
    QString type;
    QString host;
    uint16_t port = 0;
    QVariantMap extra;
};

enum class ErrorCode {
    Success = 0,
    
    AudioReadFailed = 1001, //暂时未使用
    AudioDeviceNotFound = 1002,
    AudioDeviceInitFailed = 1003,
    InvalidState = 1004,
    UnsupportedFormat = 1005,
    UnknownSourceType = 1006,

    ProcessingFailed = 2001,
    
    StorageFull = 3001,
    FileWriteFailed = 3002,
    
    NetworkTimeout = 4001,
    NetworkPortOccupied = 4002,
    NetworkListenFailed = 4003,
    DatabaseInitFailed = 4004,
    DatabaseConnectionFailed = 4005,
    InvalidConfig = 4006,
    ConfigMissingField = 4007,
    DatabaseQueryFailed = 4008,
    NetworkFileIOFailed = 4009,
    RecordNotFound = 4010,
    HttpServerError = 4011,
    AuthorizationFailed = 4012,
    FileNotExist = 4013,
    InvalidParam = 4014,
    TaskProcessingFailed = 4015,
    BatchDownloadFailed = 4016,
    ToolsError = 4017
};

template<typename T>
class Result {
public:
    static Result<T> ok(const T& value) {
        Result r;
        r.m_value = value;
        r.m_ok = true;
        return r;
    }

    static Result<T> error(const QString& message, ErrorCode code = ErrorCode::Success) {
        Result r;
        r.m_errorMessage = message;
        r.m_errorCode = code;
        r.m_ok = false;
        return r;
    }

    bool isOk() const { return m_ok; }
    const T& value() const { return m_value; }
    QString errorMessage() const { return m_errorMessage; }
    ErrorCode errorCode() const { return m_errorCode; }

private:
    T m_value{};
    QString m_errorMessage;
    ErrorCode m_errorCode = ErrorCode::Success;
    bool m_ok = false;
};

template<>
class Result<void> {
public:
    static Result<void> ok() {
        Result r;
        r.m_ok = true;
        return r;
    }

    static Result<void> error(const QString& message, ErrorCode code = ErrorCode::Success) {
        Result r;
        r.m_errorMessage = message;
        r.m_errorCode = code;
        r.m_ok = false;
        return r;
    }

    bool isOk() const { return m_ok; }
    QString errorMessage() const { return m_errorMessage; }
    ErrorCode errorCode() const { return m_errorCode; }

private:
    QString m_errorMessage;
    ErrorCode m_errorCode = ErrorCode::Success;
    bool m_ok = false;
};

}

#endif