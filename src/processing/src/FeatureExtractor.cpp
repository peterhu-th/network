#include "../include/FeatureExtractor.h"
#include <QDebug>

namespace radar {

    Result<ProcessedData> FeatureExtractor::process(const AudioFrame& frame) {
        // 输入验证
        auto validateResult = validateInput(frame);
        if (!validateResult.isOk()) {
            return Result<ProcessedData>::error(validateResult.errorMessage(), validateResult.errorCode());
        }

        // 提取音频特征
        ProcessedData output;
        output.originalFrame = frame;
        output.isValid = true;
        output.signalStrength = 0.8f;

        // 基础特征提取
        output.features["sample_rate"] = frame.sampleRate;
        output.features["audio_length_ms"] = (frame.data.size() * 1000) / (frame.sampleRate * 2);
        output.features["channel_count"] = 1;
        output.features["bit_depth"] = 16;

        return Result<ProcessedData>::ok(output);
    }

    QVariantMap FeatureExtractor::extractFeatures(const AudioFrame& frame) {
        QVariantMap features;
        features["sample_rate"] = frame.sampleRate;
        features["data_size"] = frame.data.size();
        features["timestamp"] = frame.timestamp;
        features["source_id"] = frame.sourceId;
        return features;
    }

} // namespace radar