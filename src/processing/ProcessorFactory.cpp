#include "ProcessorFactory.h"
#include "DenoiseProcessor.h"
#include "VADProcessor.h"
#include "FeatureExtractor.h"

namespace radar {

    // 实现创建方法（补全函数体，解决语法报错）
    std::unique_ptr<Processor> ProcessorFactory::createProcessor(ProcessorType type) {
        switch (type) {
        case ProcessorType::Denoise:
            return std::make_unique<DenoiseProcessor>();
        case ProcessorType::VAD:
            return std::make_unique<VADProcessor>();
        case ProcessorType::FeatureExtractor:
            return std::make_unique<FeatureExtractor>();
        default:
            return nullptr;
        }
    }

} // namespace radar