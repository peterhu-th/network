#ifndef FEATURE_EXTRACTOR_H
#define FEATURE_EXTRACTOR_H

#include "radar_processor_base.h"

namespace radar {

    class FeatureExtractor : public AudioProcessorBase {
    protected:
        Result<ProcessedData> doProcess(const AudioFrame& input) override;

    private:
        int16_t calculateMaxAmplitude(const AudioFrame& frame);
        double calculateSNR(const AudioFrame& frame);
    };

} // namespace radar

#endif // FEATURE_EXTRACTOR_H