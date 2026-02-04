#ifndef PROCESSING_PROCESSOR_FACTORY_H
#define PROCESSING_PROCESSOR_FACTORY_H

#include "radar_processor_base.h"
#include "DenoiseProcessor.h"
#include "VADProcessor.h"
#include "FeatureExtractor.h"
#include <memory>
#include <vector>

namespace radar {

    class ProcessorFactory {
    public:
        static std::unique_ptr<Processor> createDenoiseProcessor();
        static std::unique_ptr<Processor> createVADProcessor(double threshold = 800.0);
        static std::unique_ptr<Processor> createFeatureExtractor();
        static std::vector<std::unique_ptr<Processor>> createDefaultPipeline();

    private:
        ProcessorFactory() = default;
    };

} // namespace radar

#endif // PROCESSING_PROCESSOR_FACTORY_H