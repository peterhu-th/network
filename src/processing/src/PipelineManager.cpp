#include "../include/PipelineManager.h"
#include <QDebug>

namespace radar {

    void PipelineManager::addProcessor(std::unique_ptr<Processor> processor) {
        if (processor) {
            m_processors.push_back(std::move(processor));
        }
    }

    Result<ProcessedData> PipelineManager::execute(const AudioFrame& frame) {
        if (m_processors.empty()) {
            return Result<ProcessedData>::error("处理器流水线为空", ErrorCode::ProcessingFailed);
        }

        ProcessedData finalResult;
        finalResult.originalFrame = frame;

        // 依次执行所有处理器
        for (const auto& processor : m_processors) {
            auto result = processor->process(frame);
            if (!result.isOk()) {
                return Result<ProcessedData>::error(
                    "处理器执行失败: " + result.errorMessage(),
                    result.errorCode()
                );
            }
            // 合并特征和状态
            finalResult.isValid &= result.value().isValid;
            finalResult.signalStrength = (finalResult.signalStrength + result.value().signalStrength) / 2;
            finalResult.features.unite(result.value().features);
        }

        return Result<ProcessedData>::ok(finalResult);
    }

} // namespace radar