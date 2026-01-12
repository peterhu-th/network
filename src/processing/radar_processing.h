#ifndef RADAR_PROCESSING_H
#define RADAR_PROCESSING_H

#include <vector>
#include <memory>
#include <string>
#include <QByteArray>  // 若无需Qt，替换为 #include <vector> 并将QByteArray改为std::vector<char>
#include <cmath>
#include <algorithm>

// 命名空间隔离，避免命名冲突
namespace radar {

// 错误码枚举（覆盖模块核心错误场景）
enum class ErrorCode {
    Success = 0,        // 成功
    InvalidData = 1,    // 输入数据无效
    ProcessError = 2,   // 处理逻辑错误
    PipelineEmpty = 3   // 流水线无处理器
};

// 通用返回结果类（支持泛型，统一错误处理）
template <typename T>
class Result {
public:
    // 静态方法：创建成功结果
    static Result<T> ok(const T& data) {
        return Result(true, data, "", ErrorCode::Success);
    }

    // 静态方法：创建错误结果
    static Result<T> error(const std::string& msg, ErrorCode code) {
        return Result(false, T(), msg, code);
    }

    // 检查是否成功
    bool isOk() const { return is_ok_; }

    // 获取数据（仅成功时有效）
    T value() const { return data_; }

    // 获取错误信息
    std::string errorMsg() const { return error_msg_; }

    // 获取错误码
    ErrorCode errorCode() const { return error_code_; }

private:
    Result(bool is_ok, const T& data, const std::string& msg, ErrorCode code)
        : is_ok_(is_ok), data_(data), error_msg_(msg), error_code_(code) {}

    bool is_ok_;          // 是否成功
    T data_;              // 结果数据
    std::string error_msg_; // 错误信息
    ErrorCode error_code_;  // 错误码
};

// 音频帧基础结构（输入数据）
struct AudioFrame {
    uint64_t timestamp;   // 时间戳（毫秒）
    int sample_rate;      // 采样率（Hz）
    int channels;         // 声道数
    int bit_depth;        // 位深（如16bit）
    QByteArray data;      // 音频原始数据（PCM）
                          // 无Qt时替换为：std::vector<char> data;
};

// 处理后的数据结构（输出数据）
struct ProcessedData {
    AudioFrame original_frame;  // 原始音频帧
    AudioFrame processed_frame; // 处理后的音频帧
    bool is_valid;              // 是否为有效信号（VAD结果）
    double signal_strength;     // 信号强度（能量值）
    std::map<std::string, double> features; // 特征数据（如最大振幅、信噪比）
};

// 处理器抽象基类（定义统一接口，符合开闭原则）
class Processor {
public:
    virtual ~Processor() = default; // 虚析构，保证子类正确析构

    // 核心处理接口：输入音频帧，输出处理结果
    virtual Result<ProcessedData> process(const AudioFrame& input) = 0;
};

// 流水线管理器（管理处理器执行顺序，单一职责：调度处理器）
class PipelineManager {
public:
    // 添加处理器到流水线（智能指针管理，避免内存泄漏）
    void addProcessor(std::unique_ptr<Processor> processor);

    // 执行流水线处理
    Result<ProcessedData> execute(const AudioFrame& input);

    // 清空流水线
    void clearProcessors() { processors_.clear(); }

private:
    // 处理器列表（串行执行）
    std::vector<std::unique_ptr<Processor>> processors_;
};

// 降噪处理器（具体实现：均值滤波降噪）
class DenoiseProcessor : public Processor {
public:
    Result<ProcessedData> process(const AudioFrame& input) override;

private:
    // 均值滤波核心逻辑（邻域3点均值）
    QVector<int16_t> meanFilter(const QVector<int16_t>& pcm_data);
};

// 静音检测处理器（VAD，基于能量阈值）
class VADProcessor : public Processor {
public:
    // 构造函数：支持配置能量阈值（可外部自定义）
    explicit VADProcessor(double energy_threshold = 1000.0)
        : energy_threshold_(energy_threshold) {}

    Result<ProcessedData> process(const AudioFrame& input) override;

private:
    // 计算音频帧能量（平方和均值）
    double calculateEnergy(const AudioFrame& frame);
    double energy_threshold_; // 静音判断阈值
};

// 特征提取处理器（提取最大振幅、信噪比等特征）
class FeatureExtractor : public Processor {
public:
    Result<ProcessedData> process(const AudioFrame& input) override;

private:
    // 计算最大振幅
    int16_t calculateMaxAmplitude(const AudioFrame& frame);
    // 计算信噪比（简化实现，实际可根据降噪前后数据优化）
    double calculateSNR(const AudioFrame& frame);
};

} // namespace radar

#endif // RADAR_PROCESSING_H