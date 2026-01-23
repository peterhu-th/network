
#ifndef RADAR_TYPES_H
#define RADAR_TYPES_H

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QVariantMap>
#include <cstdint>

namespace radar {

    enum class ErrorCode {
        Success = 0,
        InvalidData = 1,
        ProcessError = 2,
        PipelineEmpty = 3
    };

    template<typename T>
    class Result {
    public:
        static Result<T> ok(const T& value);
        static Result<T> error(const QString& message, ErrorCode code);
        bool isOk() const;
        T value() const;

    private:
        Result(bool is_ok, const T& data, const QString& msg, ErrorCode code)
            : is_ok_(is_ok), data_(data), error_msg_(msg), error_code_(code) {}
        bool is_ok_;
        T data_;
        QString error_msg_;
        ErrorCode error_code_;
    };

    struct AudioFrame {
        int64_t timestamp;       // 时间戳（微秒）
        uint32_t sampleRate;     // 采样率
        uint16_t channels;       // 声道数
        uint16_t bitsPerSample;  // 位深
        QByteArray data;         // PCM原始数据
        QString sourceId;        // 数据源标识
    };

    struct ProcessedData {
        AudioFrame originalFrame;
        bool isValid;            // 是否有效信号
        double signalStrength;   // 信号强度
        QVariantMap features;    // 特征数据
    };

    // Result类实现
    template<typename T>
    Result<T> Result<T>::ok(const T& value) {
        return Result(true, value, "", ErrorCode::Success);
    }

    template<typename T>
    Result<T> Result<T>::error(const QString& message, ErrorCode code) {
        return Result(false, T(), message, code);
    }

    template<typename T>
    bool Result<T>::isOk() const {
        return is_ok_;
    }

    template<typename T>
    T Result<T>::value() const {
        return data_;
    }

} // namespace radar

#endif // RADAR_TYPES_H