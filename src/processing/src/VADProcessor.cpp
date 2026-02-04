#include "../include/VADProcessor.h"

namespace radar {

    VADProcessor::VADProcessor(double threshold) : m_threshold(threshold) {}

    Result<ProcessedData> VADProcessor::process(const AudioFrame& frame) {
        auto validateRes = validateInput(frame);
        if (!validateRes.isOk()) {
            // 替换 InvalidData 为 ProcessingFailed
            return Result<ProcessedData>::error("VAD处理器：输入无效", ErrorCode::ProcessingFailed);
        }

        ProcessedData output;
        output.originalFrame = frame;
        output.isValid = detectVoice(frame);
        output.signalStrength = output.isValid ? 0.88 : 0.15;
        output.features.insert("vad_threshold", m_threshold);
        output.features.insert("is_voice_detected", output.isValid);

        return Result<ProcessedData>::ok(output);
    }

    bool VADProcessor::detectVoice(const AudioFrame& frame) {
        Q_UNUSED(frame);
        Q_UNUSED(m_threshold);
        return true;
    }

} // namespace radar