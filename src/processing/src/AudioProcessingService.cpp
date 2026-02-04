#include "../include/AudioProcessingService.h"
#include "../include/ProcessorFactory.h"

namespace radar {

    AudioProcessingService::AudioProcessingService() {
        initDefaultPipeline();
    }

    void AudioProcessingService::initDefaultPipeline() {
        // 创建默认处理器流水线
        auto denoiser = std::make_unique<DenoiseProcessor>();
        auto vad = std::make_unique<VADProcessor>(0.5f);
        auto featureExtractor = std::make_unique<FeatureExtractor>();

        m_pipeline.addProcessor(std::move(denoiser));
        m_pipeline.addProcessor(std::move(vad));
        m_pipeline.addProcessor(std::move(featureExtractor));
    }

    Result<ProcessedData> AudioProcessingService::processAudio(const AudioFrame& frame) {
        return m_pipeline.execute(frame);
    }

} // namespace radar