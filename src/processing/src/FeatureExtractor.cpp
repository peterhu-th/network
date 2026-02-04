#include "../include/FeatureExtractor.h"

namespace radar {

    Result<ProcessedData> FeatureExtractor::process(const AudioFrame& frame) {
        auto validateRes = validateInput(frame);
        if (!validateRes.isOk()) {
            // 替换 InvalidData 为 ProcessingFailed
            return Result<ProcessedData>::error("特征提取器：输入无效", ErrorCode::ProcessingFailed);
        }

        ProcessedData output;
        output.originalFrame = frame;
        output.isValid = true;
        output.signalStrength = 0.90;
        output.features = extractFeatures(frame);

        return Result<ProcessedData>::ok(output);
    }

    QVariantMap FeatureExtractor::extractFeatures(const AudioFrame& frame) {
        QVariantMap features;
        features.insert("frame_size", frame.data.size());
        features.insert("sample_rate", frame.sampleRate);
        features.insert("signal_energy", 1250.5);
        return features;
    }

} // namespace radar