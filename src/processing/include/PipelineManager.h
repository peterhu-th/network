#ifndef PROCESSING_PIPELINEMANAGER_H
#define PROCESSING_PIPELINEMANAGER_H

#include <vector>
#include <memory>
#include "radar_processor_base.h"
#include "../../core/Types.h"

namespace radar {

    class PipelineManager {
    public:
        void addProcessor(std::unique_ptr<Processor> processor);
        Result<ProcessedData> execute(const AudioFrame& frame);
        void clearProcessors(); // 新增清空方法

    private:
        std::vector<std::unique_ptr<Processor>> m_processors;
    };

} // namespace radar

#endif // PROCESSING_PIPELINEMANAGER_H