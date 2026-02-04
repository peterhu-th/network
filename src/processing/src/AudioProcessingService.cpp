#include "../include/AudioProcessingService.h"

namespace radar {

    AudioProcessingService::AudioProcessingService() {
        auto defaultProcessors = ProcessorFactory::createDefaultPipeline();
        for (auto& p : defaultProcessors) {
            m_pipeline.addProcessor(std::move(p));
        }
    }

    AudioProcessingService::AudioProcessingService(std::vector<std::unique_ptr<Processor>> customProcessors) {
        for (auto& p : customProcessors) {
            m_pipeline.addProcessor(std::move(p));
        }
    }

    std::unique_ptr<AudioProcessingService> AudioProcessingService::createCustomService(std::vector<std::unique_ptr<Processor>> customProcessors) {
        return std::make_unique<AudioProcessingService>(std::move(customProcessors));
    }

    Result<ProcessedData> AudioProcessingService::processAudioFrame(const AudioFrame& frame) {
        return m_pipeline.execute(frame);
    }

} // namespace radar