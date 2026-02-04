#include "../include/PipelineManager.h"

namespace radar {

    void PipelineManager::addProcessor(std::unique_ptr<Processor> processor) {
        m_processors.push_back(std::move(processor));
    }

    Result<ProcessedData> PipelineManager::execute(const AudioFrame& input) {
        ProcessedData currentData;
        currentData.originalFrame = input;
        currentData.isValid = true;

        for (auto& processor : m_processors) {
            if (!currentData.isValid) {
                // 替换 PipelineError 为 ProcessingFailed
                return Result<ProcessedData>::error("流水线：无效数据终止", ErrorCode::ProcessingFailed);
            }

            auto processRes = processor->process(currentData.originalFrame);
            if (!processRes.isOk()) {
                // 替换 ProcessorError 为 ProcessingFailed
                return Result<ProcessedData>::error("流水线：处理器执行失败", ErrorCode::ProcessingFailed);
            }

            currentData = processRes.value();
        }

        return Result<ProcessedData>::ok(currentData);
    }

} // namespace radar