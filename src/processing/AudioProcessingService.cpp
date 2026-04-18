#include "AudioProcessingService.h"
#include "ProcessorFactory.h"

namespace radar {

    void AudioProcessingService::initDefaultPipeline() {
        initPipelineForFullFeature();
    }

    Result<ProcessedData> AudioProcessingService::processAudio(const AudioFrame& frame) {
        return m_pipeline.execute(frame);
    }

    void AudioProcessingService::initPipelineForVoice() {
        m_pipeline.clearProcessors();
        auto denoise = ProcessorFactory::createProcessor(ProcessorFactory::ProcessorType::Denoise);
        auto vad = ProcessorFactory::createProcessor(ProcessorFactory::ProcessorType::VAD);
        if (denoise) m_pipeline.addProcessor(std::move(denoise));
        if (vad) m_pipeline.addProcessor(std::move(vad));
    }

    void AudioProcessingService::initPipelineForFullFeature() {
        m_pipeline.clearProcessors();
        auto denoise = ProcessorFactory::createProcessor(ProcessorFactory::ProcessorType::Denoise);
        auto vad = ProcessorFactory::createProcessor(ProcessorFactory::ProcessorType::VAD);
        auto feature = ProcessorFactory::createProcessor(ProcessorFactory::ProcessorType::FeatureExtractor);
        if (denoise) m_pipeline.addProcessor(std::move(denoise));
        if (vad) m_pipeline.addProcessor(std::move(vad));
        if (feature) m_pipeline.addProcessor(std::move(feature));
    }

    void AudioProcessingService::initPipelineForSimpleDenoise() {
        m_pipeline.clearProcessors();
        auto denoise = ProcessorFactory::createProcessor(ProcessorFactory::ProcessorType::Denoise);
        if (denoise) m_pipeline.addProcessor(std::move(denoise));
    }

} // namespace radar