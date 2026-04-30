#include "ProcessorFactory.h"
#include "DenoiseProcessor.h"
#include "VADProcessor.h"
#include "FeatureExtractor.h"
#include "Config.h"

namespace radar {

    std::unique_ptr<Processor> ProcessorFactory::createProcessor(ProcessorType type) {
        switch (type) {
        case ProcessorType::Denoise:
            return std::make_unique<DenoiseProcessor>(Config::instance().denoiseConfig());
        case ProcessorType::VAD:
            return std::make_unique<VADProcessor>();
        case ProcessorType::FeatureExtractor:
            return std::make_unique<FeatureExtractor>();
        default:
            return nullptr;
        }
    }

}
