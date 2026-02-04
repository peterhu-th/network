#ifndef PROCESSING_DENOISEPROCESSOR_H
#define PROCESSING_DENOISEPROCESSOR_H

#include "radar_processor_base.h"
#include "../../src/core/types.h"

namespace radar {

    class DenoiseProcessor : public Processor {
    public:
        Result<ProcessedData> process(const AudioFrame& frame) override;

    private:
        // 简单的均值去噪算法
        QByteArray denoise(const QByteArray& rawData);
    };

} // namespace radar

#endif // PROCESSING_DENOISEPROCESSOR_H