#ifndef PROCESSING_VADPROCESSOR_H
#define PROCESSING_VADPROCESSOR_H

#include "radar_processor_base.h"
#include "../../src/core/types.h"

namespace radar {

    class VADProcessor : public Processor {
    public:
        explicit VADProcessor(float threshold = 0.5f) : m_threshold(threshold) {}
        Result<ProcessedData> process(const AudioFrame& frame) override;

    private:
        float m_threshold;
        // 计算音频能量（用于语音活性检测）
        float calculateEnergy(const QByteArray& audioData);
    };

} // namespace radar

#endif // PROCESSING_VADPROCESSOR_H