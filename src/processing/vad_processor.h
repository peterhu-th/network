#ifndef VAD_PROCESSOR_H
#define VAD_PROCESSOR_H

#include "radar_processor_base.h"

namespace radar {

    class VADProcessor : public AudioProcessorBase {
    public:
        explicit VADProcessor(double energy_threshold = 1000.0)
            : energy_threshold_(energy_threshold) {}

    protected:
        Result<ProcessedData> doProcess(const AudioFrame& input) override;

    private:
        double energy_threshold_;
    };

} // namespace radar

#endif // VAD_PROCESSOR_H