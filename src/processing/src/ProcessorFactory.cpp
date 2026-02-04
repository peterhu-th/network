#include "../include/ProcessorFactory.h"

namespace radar {

    std::unique_ptr<Processor> ProcessorFactory::createDenoiseProcessor() {
        return std::make_unique<DenoiseProcessor>();
    }

    std::unique_ptr<Processor> ProcessorFactory::createVADProcessor(double threshold) {
        return std::make_unique<VADProcessor>(threshold);
    }

    std::unique_ptr<Processor> ProcessorFactory::createFeatureExtractor() {
        return std::make_unique<FeatureExtractor>();
    }

    std::vector<std::unique_ptr<Processor>> ProcessorFactory::createDefaultPipeline() {
        std::vector<std::unique_ptr<Processor>> processors;
        processors.push_back(createDenoiseProcessor());
        processors.push_back(createVADProcessor());
        processors.push_back(createFeatureExtractor());
        return processors;
    }

} // namespace radar