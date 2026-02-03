#ifndef PIPELINE_MANAGER_H
#define PIPELINE_MANAGER_H

#include "radar_types.h"
#include "radar_processor_base.h"
#include <memory>
#include <vector>

namespace radar {

    class PipelineManager {
    public:
        void addProcessor(std::unique_ptr<Processor> processor) {
            processors_.push_back(std::move(processor));
        }

        Result<ProcessedData> execute(const AudioFrame& input) {
            ProcessedData current_data;
            current_data.originalFrame = input;
            current_data.isValid = true;

            for (auto& processor : processors_) {
                auto result = processor->process(input);
                if (!result.isOk()) {
                    return Result<ProcessedData>::error(result.errorMsg(), result.errorCode());
                }
                current_data = result.value();
                if (!current_data.isValid) break;
            }
            return Result<ProcessedData>::ok(current_data);
        }

    private:
        std::vector<std::unique_ptr<Processor>> processors_;
    };

} // namespace radar

#endif // PIPELINE_MANAGER_H