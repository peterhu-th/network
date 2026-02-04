#ifndef PROCESSING_DENOISE_PROCESSOR_H
#define PROCESSING_DENOISE_PROCESSOR_H

#include "radar_processor_base.h"

namespace radar {

    class DenoiseProcessor : public Processor {
    public:
        Result<ProcessedData> process(const AudioFrame& frame) override;

    private:
        AudioFrame applyDenoise(const AudioFrame& frame);
    };

} // namespace radar

#endif // PROCESSING_DENOISE_PROCESSOR_H