#include "PipelineManager.h"

namespace radar {

    // 实现：添加处理器到流水线（原有逻辑，无修改）
    void PipelineManager::addProcessor(std::unique_ptr<Processor> processor) {
        if (processor) {
            m_processors.push_back(std::move(processor));
        }
    }

    // 实现：执行流水线（完全修复成员访问/类型转换报错）
    Result<ProcessedData> PipelineManager::execute(const AudioFrame& frame) {
        // 初始化最终结果对象
        ProcessedData finalResult;
        finalResult.originalFrame = frame;
        finalResult.isValid = true;
        finalResult.signalStrength = 0.0f;
        QVariantMap allFeatures;

        int validProcessorCount = 0;

        // 遍历流水线中的所有处理器
        for (auto& processor : m_processors) {
            // 执行单个处理器，得到 Result<ProcessedData> 类型结果
            Result<ProcessedData> processResult = processor->process(frame);

            // 核心步骤1：从 Result 中提取 ProcessedData 实际值
            // （如果你的 Result 类取值方法是 getValue()，替换成 processResult.getValue() 即可）
            ProcessedData singleResult = processResult.value();

            // 核心步骤2：访问 ProcessedData 的成员（此时无类型错误）
            finalResult.isValid &= singleResult.isValid;               // 聚合有效性
            finalResult.signalStrength += singleResult.signalStrength; // 累加信号强度

            // 合并特征（QMap 正确插入方式）
            for (auto it = singleResult.features.begin(); it != singleResult.features.end(); ++it) {
                allFeatures.insert(it.key(), it.value());
            }

            validProcessorCount++;
        }

        // 计算平均信号强度
        if (validProcessorCount > 0) {
            finalResult.signalStrength /= validProcessorCount;
            finalResult.features = allFeatures;
        } else {
            // 无处理器时标记结果无效
            finalResult.isValid = false;
            finalResult.signalStrength = 0.0f;
        }

        // 返回封装好的成功结果
        return Result<ProcessedData>::ok(finalResult);
    }

    // 实现：清空流水线（方案二核心新增方法）
    void PipelineManager::clearProcessors() {
        m_processors.clear();
    }

} // namespace radar