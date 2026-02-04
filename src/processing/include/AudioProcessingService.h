#ifndef PROCESSING_AUDIOPROCESSINGSERVICE_H
#define PROCESSING_AUDIOPROCESSINGSERVICE_H

#include "PipelineManager.h"
// 核心类型头文件（修正路径）
#include "../../src/core/types.h"

namespace radar {

    class AudioProcessingService {
    public:
        AudioProcessingService();
        // 初始化默认流水线
        void initDefaultPipeline();
        // 处理音频帧
        Result<ProcessedData> processAudio(const AudioFrame& frame);

    private:
        PipelineManager m_pipeline;
    };

} // namespace radar

#endif // PROCESSING_AUDIOPROCESSINGSERVICE_H