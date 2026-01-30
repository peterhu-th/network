#ifndef FEATURE_EXTRACTOR_H
#define FEATURE_EXTRACTOR_H

#include "radar_processor_base.h"

namespace radar {

    class FeatureExtractor : public AudioProcessorBase {
    protected:
        Result<ProcessedData> doProcess(const AudioFrame& input) override;

    private:
        double calculateSNR(const std::vector<int16_t>& data);
        int16_t calculateMaxAmplitude(const std::vector<int16_t>& data);
    };

} // namespace radar

#endif // FEATURE_EXTRACTOR_H