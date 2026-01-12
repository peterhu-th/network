#include "radar_processing.h"

namespace radar {

// -------------------------- PipelineManager 实现 --------------------------
void PipelineManager::addProcessor(std::unique_ptr<Processor> processor) {
    if (processor) {
        processors_.push_back(std::move(processor));
    }
}

Result<ProcessedData> PipelineManager::execute(const AudioFrame& input) {
    // 校验输入数据
    if (input.data.isEmpty()) { // 无Qt时替换为：input.data.empty()
        return Result<ProcessedData>::error("Input audio data is empty", ErrorCode::InvalidData);
    }

    // 校验流水线
    if (processors_.empty()) {
        return Result<ProcessedData>::error("No processors in pipeline", ErrorCode::PipelineEmpty);
    }

    // 初始化处理数据
    ProcessedData current_data;
    current_data.original_frame = input;
    current_data.processed_frame = input; // 初始化为原始帧

    // 串行执行所有处理器
    for (auto& processor : processors_) {
        auto result = processor->process(current_data.processed_frame);
        if (!result.isOk()) {
            return Result<ProcessedData>::error(
                "Processor failed: " + result.errorMsg(),
                result.errorCode()
            );
        }
        // 更新处理后的数据
        current_data = result.value();
    }

    return Result<ProcessedData>::ok(current_data);
}

// -------------------------- DenoiseProcessor 实现 --------------------------
Result<ProcessedData> DenoiseProcessor::process(const AudioFrame& input) {
    ProcessedData output;
    output.original_frame = input;
    output.processed_frame = input;

    // 校验输入数据格式（16bit位深）
    if (input.bit_depth != 16 || input.data.size() % sizeof(int16_t) != 0) {
        return Result<ProcessedData>::error(
            "Unsupported bit depth or invalid data size",
            ErrorCode::InvalidData
        );
    }

    // 解析PCM数据（16bit）
    const int16_t* raw_data = reinterpret_cast<const int16_t*>(input.data.constData());
    int sample_count = input.data.size() / sizeof(int16_t);
    QVector<int16_t> pcm_data;
    for (int i = 0; i < sample_count; ++i) {
        pcm_data.push_back(raw_data[i]);
    }

    // 执行降噪
    QVector<int16_t> denoised_data = meanFilter(pcm_data);

    // 写回处理后的数据
    output.processed_frame.data = QByteArray(
        reinterpret_cast<const char*>(denoised_data.constData()),
        denoised_data.size() * sizeof(int16_t)
    );

    return Result<ProcessedData>::ok(output);
}

QVector<int16_t> DenoiseProcessor::meanFilter(const QVector<int16_t>& pcm_data) {
    if (pcm_data.size() <= 2) {
        return pcm_data; // 数据量过小，无需滤波
    }

    QVector<int16_t> result = pcm_data;
    // 邻域3点均值滤波（跳过首尾）
    for (int i = 1; i < pcm_data.size() - 1; ++i) {
        result[i] = static_cast<int16_t>(
            (pcm_data[i-1] + pcm_data[i] + pcm_data[i+1]) / 3
        );
    }
    return result;
}

// -------------------------- VADProcessor 实现 --------------------------
Result<ProcessedData> VADProcessor::process(const AudioFrame& input) {
    ProcessedData output;
    output.original_frame = input;
    output.processed_frame = input;

    // 计算能量
    double energy = calculateEnergy(input);
    output.signal_strength = energy;
    output.is_valid = (energy > energy_threshold_); // 有效信号判断

    return Result<ProcessedData>::ok(output);
}

double VADProcessor::calculateEnergy(const AudioFrame& frame) {
    if (frame.data.isEmpty() || frame.bit_depth != 16) {
        return 0.0;
    }

    const int16_t* data = reinterpret_cast<const int16_t*>(frame.data.constData());
    int sample_count = frame.data.size() / sizeof(int16_t);
    double sum = 0.0;

    for (int i = 0; i < sample_count; ++i) {
        sum += static_cast<double>(data[i]) * data[i];
    }

    return sum / sample_count; // 能量 = 平方和均值
}

// -------------------------- FeatureExtractor 实现 --------------------------
Result<ProcessedData> FeatureExtractor::process(const AudioFrame& input) {
    ProcessedData output;
    output.original_frame = input;
    output.processed_frame = input;

    if (input.bit_depth != 16 || input.data.isEmpty()) {
        return Result<ProcessedData>::error(
            "Invalid input for feature extraction",
            ErrorCode::InvalidData
        );
    }

    // 提取特征
    output.features["max_amplitude"] = calculateMaxAmplitude(input);
    output.features["snr"] = calculateSNR(input);
    output.features["signal_strength"] = calculateEnergy(input); // 复用VAD的能量计算

    return Result<ProcessedData>::ok(output);
}

int16_t FeatureExtractor::calculateMaxAmplitude(const AudioFrame& frame) {
    const int16_t* data = reinterpret_cast<const int16_t*>(frame.data.constData());
    int sample_count = frame.data.size() / sizeof(int16_t);
    int16_t max_amp = 0;

    for (int i = 0; i < sample_count; ++i) {
        max_amp = std::max(max_amp, static_cast<int16_t>(std::abs(data[i])));
    }

    return max_amp;
}

double FeatureExtractor::calculateSNR(const AudioFrame& frame) {
    // 简化实现：实际需对比降噪前后的能量
    double signal_energy = calculateEnergy(frame);
    double noise_energy = signal_energy * 0.1; // 假设噪声占10%
    return 10 * log10(signal_energy / (noise_energy + 1e-6)); // 避免除0
}

} // namespace radar