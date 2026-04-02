#ifndef PROCESSING_AUDIOPROCESSINGSERVICE_H
#define PROCESSING_AUDIOPROCESSINGSERVICE_H

#include "PipelineManager.h"
#include "Types.h"

namespace radar {

    class AudioProcessingService {
    public:
        AudioProcessingService() { initDefaultPipeline(); }
        Result<ProcessedData> processAudio(const AudioFrame& frame);

        // 新增流水线切换方法
        void initPipelineForVoice();
        void initPipelineForFullFeature();
        void initPipelineForSimpleDenoise();

    private:
        void initDefaultPipeline();
        PipelineManager m_pipeline;
    };

} // namespace radar

#endif // PROCESSING_AUDIOPROCESSINGSERVICE_H