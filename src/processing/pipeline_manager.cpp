#include "pipeline_manager.h"

namespace radar {

    void PipelineManager::addProcessor(std::unique_ptr<Processor> processor) {
        if (processor) {
            processors_.push_back(std::move(processor));
        }
    }

    Result<ProcessedData> PipelineManager::execute(const AudioFrame& input) {
        if (input.data.isEmpty()) {
            return Result<ProcessedData>::error("Input audio data is empty", ErrorCode::InvalidData);
        }
        if (processors_.empty()) {
            return Result<ProcessedData>::error("No processors in pipeline", ErrorCode::PipelineEmpty);
        }

        ProcessedData current_data;
        current_data.originalFrame = input;
        current_data.isValid = false;
        current_data.signalStrength = 0.0;
        current_data.features.clear();

        for (auto& processor : processors_) {
            auto result = processor->process(current_data.originalFrame);
            if (!result.isOk()) {
                return Result<ProcessedData>::error(
                    "Processor failed: " + result.errorMsg(),
                    result.errorCode()
                );
            }
            current_data = result.value();
        }

        return Result<ProcessedData>::ok(current_data);
    }

}