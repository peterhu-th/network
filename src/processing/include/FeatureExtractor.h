#ifndef PROCESSING_FEATURE_EXTRACTOR_H
#define PROCESSING_FEATURE_EXTRACTOR_H

#include "radar_processor_base.h"

namespace radar {

    class FeatureExtractor : public Processor {
    public:
        Result<ProcessedData> process(const AudioFrame& frame) override;

    private:
        QVariantMap extractFeatures(const AudioFrame& frame);
    };

} // namespace radar

#endif // PROCESSING_FEATURE_EXTRACTOR_H