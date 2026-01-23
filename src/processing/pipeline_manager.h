#ifndef PIPELINE_MANAGER_H
#define PIPELINE_MANAGER_H

#include "radar_processor_base.h"
#include <vector>
#include <memory>

namespace radar {

    class PipelineManager {
    public:
        void addProcessor(std::unique_ptr<Processor> processor);
        Result<ProcessedData> execute(const AudioFrame& input);
        void clearProcessors() { processors_.clear(); }

    private:
        std::vector<std::unique_ptr<Processor>> processors_;
    };

} // namespace radar

#endif // PIPELINE_MANAGER_H