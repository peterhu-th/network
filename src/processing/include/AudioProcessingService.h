#ifndef PROCESSING_AUDIO_PROCESSING_SERVICE_H
#define PROCESSING_AUDIO_PROCESSING_SERVICE_H

#include "PipelineManager.h"
#include "ProcessorFactory.h"
#include "../core/types.h"
#include <vector>
#include <memory>

namespace radar {

    class AudioProcessingService {
    public:
        AudioProcessingService();
        explicit AudioProcessingService(std::vector<std::unique_ptr<Processor>> customProcessors);
        static std::unique_ptr<AudioProcessingService> createCustomService(std::vector<std::unique_ptr<Processor>> customProcessors);
        Result<ProcessedData> processAudioFrame(const AudioFrame& frame);

    private:
        PipelineManager m_pipeline;
    };

} // namespace radar

#endif // PROCESSING_AUDIO_PROCESSING_SERVICE_H