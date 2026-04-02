#ifndef PROCESSING_FEATUREEXTRACTOR_H
#define PROCESSING_FEATUREEXTRACTOR_H

#include "radar_processor_base.h"
// 核心类型头文件（修正路径）
#include "../../src/core/types.h"

namespace radar {

    class FeatureExtractor : public Processor {
    public:
        Result<ProcessedData> process(const AudioFrame& frame) override;

    private:
        // 提取音频特征（如帧率、能量、时长等）
        QVariantMap extractFeatures(const AudioFrame& frame);
    };

} // namespace radar

#endif // PROCESSING_FEATUREEXTRACTOR_H