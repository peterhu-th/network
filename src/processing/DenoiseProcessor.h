#ifndef DENOISE_PROCESSOR_H
#define DENOISE_PROCESSOR_H

#include "radar_processor_base.h"

namespace radar {

    class DenoiseProcessor : public AudioProcessorBase {
    protected:
        Result<ProcessedData> doProcess(const AudioFrame& input) override;

    private:
        std::vector<int16_t> meanFilter(const std::vector<int16_t>& data);
    };

} // namespace radar

#endif // DENOISE_PROCESSOR_H