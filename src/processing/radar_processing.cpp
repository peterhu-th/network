#include "radar_processing.h"

namespace radar {
// 工具函数：计算音频帧能量（平方和均值）
double calculateEnergy(const AudioFrame& frame) {
    if (frame.data.empty() || frame.bit_depth != 16) {
        return 0.0;
    }
    const int16_t* data = reinterpret_cast<const int16_t*>(frame.data.data());
    int sample_count = frame.data.size() / sizeof(int16_t);
    double sum = 0.0;
    for (int i = 0; i < sample_count; ++i) {
        sum += static_cast<double>(data[i]) * data[i];
    }
    return sum / sample_count;
}

// -------------------------- PipelineManager 实现 --------------------------
void PipelineManager::addProcessor(std::unique_ptr<Processor> processor) {
    if (processor) {
        processors_.push_back(std::move(processor));
    }
}

Result<ProcessedData> PipelineManager::execute(const AudioFrame& input) {
    if (input.data.empty()) {
        return Result<ProcessedData>::error("Input audio data is empty", ErrorCode::InvalidData);
    }
    if (processors_.empty()) {
        return Result<ProcessedData>::error("No processors in pipeline", ErrorCode::PipelineEmpty);
    }

    ProcessedData current_data;
    current_data.original_frame = input;
    current_data.processed_frame = input;

    for (auto& processor : processors_) {
        auto result = processor->process(current_data.processed_frame);
        if (!result.isOk()) {
            return Result<ProcessedData>::error(
                "Processor failed: " + result.errorMsg(),
                result.errorCode()
            );
        }
        current_data = result.value();
    }
    return Result<ProcessedData>::ok(current_data);
}

// -------------------------- DenoiseProcessor 实现 --------------------------
Result<ProcessedData> DenoiseProcessor::process(const AudioFrame& input) {
    ProcessedData output;
    output.original_frame = input;
    output.processed_frame = input;

    if (input.bit_depth != 16 || input.data.size() % sizeof(int16_t) != 0) {
        return Result<ProcessedData>::error(
            "Only 16bit PCM is supported",
            ErrorCode::InvalidData
        );
    }

    const int16_t* raw_data = reinterpret_cast<const int16_t*>(input.data.data());
    int sample_count = input.data.size() / sizeof(int16_t);
    std::vector<int16_t> pcm_data(raw_data, raw_data + sample_count);
    std::vector<int16_t> denoised_data = meanFilter(pcm_data);

    // 写回处理后的数据
    output.processed_frame.data.resize(denoised_data.size() * sizeof(int16_t));
    memcpy(output.processed_frame.data.data(), denoised_data.data(), output.processed_frame.data.size());
    return Result<ProcessedData>::ok(output);
}

std::vector<int16_t> DenoiseProcessor::meanFilter(const std::vector<int16_t>& pcm_data) {
    if (pcm_data.size() <= 2) {
        return pcm_data;
    }
    std::vector<int16_t> result = pcm_data;
    for (int i = 1; i < pcm_data.size() - 1; ++i) {
        result[i] = static_cast<int16_t>((pcm_data[i-1] + pcm_data[i] + pcm_data[i+1]) / 3);
    }
    return result;
}

// -------------------------- VADProcessor 实现 --------------------------
Result<ProcessedData> VADProcessor::process(const AudioFrame& input) {
    ProcessedData output;
    output.original_frame = input;
    output.processed_frame = input;

    double energy = calculateEnergy(input);
    output.signal_strength = energy;
    output.is_valid = (energy > energy_threshold_);
    return Result<ProcessedData>::ok(output);
}

// -------------------------- FeatureExtractor 实现 --------------------------
Result<ProcessedData> FeatureExtractor::process(const AudioFrame& input) {
    ProcessedData output;
    output.original_frame = input;
    output.processed_frame = input;

    if (input.bit_depth != 16 || input.data.empty()) {
        return Result<ProcessedData>::error(
            "Invalid input for feature extraction",
            ErrorCode::InvalidData
        );
    }

    // 提取特征（无冗余）
    output.features["max_amplitude"] = calculateMaxAmplitude(input);
    output.features["snr"] = calculateSNR(output.original_frame, output.processed_frame);
    return Result<ProcessedData>::ok(output);
}

int16_t FeatureExtractor::calculateMaxAmplitude(const AudioFrame& frame) {
    const int16_t* data = reinterpret_cast<const int16_t*>(frame.data.data());
    int sample_count = frame.data.size() / sizeof(int16_t);
    int16_t max_amp = 0;
    for (int i = 0; i < sample_count; ++i) {
        max_amp = std::max(max_amp, static_cast<int16_t>(std::abs(data[i])));
    }
    return max_amp;
}

double FeatureExtractor::calculateSNR(const AudioFrame& original, const AudioFrame& processed) {
    double original_energy = calculateEnergy(original);
    double processed_energy = calculateEnergy(processed);
    double noise_energy = original_energy - processed_energy;
    return 10 * log10(processed_energy / (noise_energy + 1e-6)); // 避免除0
}
} // namespace radar
