#ifndef PROCESSING_PIPELINEMANAGER_H
#define PROCESSING_PIPELINEMANAGER_H

#include <memory>
#include <vector>
#include "radar_processor_base.h"
// 核心类型头文件（修正路径）
#include "../../src/core/types.h"

namespace radar {

    class PipelineManager {
    public:
        // 添加处理器到流水线
        void addProcessor(std::unique_ptr<Processor> processor);
        // 执行流水线处理
        Result<ProcessedData> execute(const AudioFrame& frame);

    private:
        std::vector<std::unique_ptr<Processor>> m_processors;
    };

} // namespace radar

#endif // PROCESSING_PIPELINEMANAGER_H