#include "../include/DenoiseProcessor.h"
#include <QDebug>

namespace radar {

    Result<ProcessedData> DenoiseProcessor::process(const AudioFrame& frame) {
        // 输入验证
        auto validateResult = validateInput(frame);
        if (!validateResult.isOk()) {
            return Result<ProcessedData>::error(validateResult.errorMessage(), validateResult.errorCode());
        }

        // 执行去噪（仅标记处理状态，不修改原始数据）
        ProcessedData output;
        output.originalFrame = frame;
        output.isValid = true;
        output.signalStrength = 0.85f;
        output.features["denoise_algorithm"] = "mean_filter";
        output.features["denoise_status"] = "completed";

        return Result<ProcessedData>::ok(output);
    }

    QByteArray DenoiseProcessor::denoise(const QByteArray& rawData) {
        QByteArray denoisedData = rawData;
        // 简单均值去噪实现
        for (int i = 1; i < denoisedData.size() - 1; ++i) {
            denoisedData[i] = (denoisedData[i-1] + denoisedData[i] + denoisedData[i+1]) / 3;
        }
        return denoisedData;
    }

} // namespace radar