#include "../include/DenoiseProcessor.h"

namespace radar {

    Result<ProcessedData> DenoiseProcessor::process(const AudioFrame& frame) {
        auto validateRes = validateInput(frame);
        if (!validateRes.isOk()) {
            // 替换 InvalidData 为 ProcessingFailed
            return Result<ProcessedData>::error("去噪处理器：输入无效", ErrorCode::ProcessingFailed);
        }

        ProcessedData output;
        output.originalFrame = applyDenoise(frame);
        output.isValid = true;
        output.signalStrength = 0.92;
        output.features.insert("denoise_algorithm", "gaussian_filter");

        return Result<ProcessedData>::ok(output);
    }

    AudioFrame DenoiseProcessor::applyDenoise(const AudioFrame& frame) {
        AudioFrame denoisedFrame = frame;
        return denoisedFrame;
    }

} // namespace radar