#ifndef PROCESSING_PIPELINE_MANAGER_H
#define PROCESSING_PIPELINE_MANAGER_H

#include "radar_processor_base.h"
#include "../core/types.h"
#include <vector>
#include <memory>

namespace radar {

    class PipelineManager {
    public:
        void addProcessor(std::unique_ptr<Processor> processor);
        Result<ProcessedData> execute(const AudioFrame& input);

    private:
        std::vector<std::unique_ptr<Processor>> m_processors;
    };

} // namespace radar

#endif // PROCESSING_PIPELINE_MANAGER_H