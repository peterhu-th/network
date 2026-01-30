#ifndef VAD_PROCESSOR_H
#define VAD_PROCESSOR_H

#include "radar_processor_base.h"

namespace radar {

    class VADProcessor : public AudioProcessorBase {
    public:
        explicit VADProcessor(double threshold = 800.0) : threshold_(threshold) {}

    protected:
        Result<ProcessedData> doProcess(const AudioFrame& input) override;

    private:
        double threshold_;
        double calculateRMS(const std::vector<int16_t>& data);
    };

} // namespace radar

#endif // VAD_PROCESSOR_H