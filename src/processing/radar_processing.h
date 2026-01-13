#ifndef RADAR_PROCESSING_H
#define RADAR_PROCESSING_H
#include <vector>
#include <memory>
#include <string>
#include <map>
#include <cmath>
#include <algorithm>

namespace radar {
// 错误码枚举（明确场景映射，支持扩展）
enum class ErrorCode {
    Success = 0,        // 操作成功
    InvalidData = 1,    // 输入数据无效（空数据/格式不支持）
    ProcessError = 2,   // 处理逻辑异常（滤波/特征提取失败）
    PipelineEmpty = 3   // 流水线未添加处理器
};

// 通用返回结果类（泛型设计，统一错误处理）
template <typename T>
class Result {
public:
    static Result<T> ok(const T& data) {
        return Result(true, data, "", ErrorCode::Success);
    }
    static Result<T> error(const std::string& msg, ErrorCode code) {
        return Result(false, T(), msg, code);
    }
    bool isOk() const { return is_ok_; }
    T value() const { return data_; }
    std::string errorMsg() const { return error_msg_; }
    ErrorCode errorCode() const { return error_code_; }
private:
    Result(bool is_ok, const T& data, const std::string& msg, ErrorCode code)
        : is_ok_(is_ok), data_(data), error_msg_(msg), error_code_(code) {}
    bool is_ok_;
    T data_;
    std::string error_msg_;
    ErrorCode error_code_;
};

// 音频帧基础结构（输入数据）
struct AudioFrame {
    uint64_t timestamp;   // 时间戳（毫秒）
    int sample_rate;      // 采样率（Hz）
    int channels;         // 声道数
    int bit_depth;        // 位深（仅支持16bit）
    std::vector<char> data; // 音频原始数据（PCM）
};

// 处理后的数据结构（输出数据）
struct ProcessedData {
    AudioFrame original_frame;  // 原始音频帧
    AudioFrame processed_frame; // 处理后的音频帧
    bool is_valid;              // 是否为有效信号（VAD结果）
    double signal_strength;     // 信号强度（能量值）
    std::map<std::string, double> features; // 特征数据（最大振幅、信噪比）
};

// 工具函数：计算音频帧能量（抽离重复逻辑）
double calculateEnergy(const AudioFrame& frame);

// 处理器抽象基类（开闭原则）
class Processor {
public:
    virtual ~Processor() = default;
    virtual Result<ProcessedData> process(const AudioFrame& input) = 0;
};

// 流水线管理器（调度核心）
class PipelineManager {
public:
    void addProcessor(std::unique_ptr<Processor> processor);
    Result<ProcessedData> execute(const AudioFrame& input);
    void clearProcessors() { processors_.clear(); }
private:
    std::vector<std::unique_ptr<Processor>> processors_;
};

// 降噪处理器（3点均值滤波）
class DenoiseProcessor : public Processor {
public:
    Result<ProcessedData> process(const AudioFrame& input) override;
private:
    std::vector<int16_t> meanFilter(const std::vector<int16_t>& pcm_data);
};

// 静音检测处理器（VAD）
class VADProcessor : public Processor {
public:
    explicit VADProcessor(double energy_threshold = 1000.0)
        : energy_threshold_(energy_threshold) {}
    Result<ProcessedData> process(const AudioFrame& input) override;
private:
    double energy_threshold_;
};

// 特征提取处理器
class FeatureExtractor : public Processor {
public:
    Result<ProcessedData> process(const AudioFrame& input) override;
private:
    int16_t calculateMaxAmplitude(const AudioFrame& frame);
    double calculateSNR(const AudioFrame& original, const AudioFrame& processed);
};
} // namespace radar

#endif // RADAR_PROCESSING_H
