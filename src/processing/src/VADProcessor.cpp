#include "../include/VADProcessor.h"
#include <QDebug>
#include <cmath>

namespace radar {

    Result<ProcessedData> VADProcessor::process(const AudioFrame& frame) {
        // 输入验证
        auto validateResult = validateInput(frame);
        if (!validateResult.isOk()) {
            return Result<ProcessedData>::error(validateResult.errorMessage(), validateResult.errorCode());
        }

        // 计算音频能量并判断是否为有效语音
        float energy = calculateEnergy(frame.data);
        bool isVoice = energy > m_threshold;

        ProcessedData output;
        output.originalFrame = frame;
        output.isValid = isVoice;
        output.signalStrength = isVoice ? 0.9f : 0.2f;
        output.features["vad_threshold"] = m_threshold;
        output.features["audio_energy"] = energy;
        output.features["is_voice"] = isVoice;

        return Result<ProcessedData>::ok(output);
    }

    float VADProcessor::calculateEnergy(const QByteArray& audioData) {
        float energy = 0.0f;
        for (char byte : audioData) {
            short sample = static_cast<short>(static_cast<unsigned char>(byte));
            energy += sample * sample;
        }
        return std::sqrt(energy / (audioData.size() > 0 ? audioData.size() : 1));
    }

} // namespace radar