#ifndef PROCESSING_PROCESSORFACTORY_H
#define PROCESSING_PROCESSORFACTORY_H

#include <memory>
#include "radar_processor_base.h"

namespace radar {

    // 提前声明处理器类（避免头文件循环依赖）
    class DenoiseProcessor;
    class VADProcessor;
    class FeatureExtractor;

    class ProcessorFactory {
    public:
        // 处理器类型枚举（解决"未声明"报错）
        enum class ProcessorType {
            Denoise,
            VAD,
            FeatureExtractor
        };

        // 静态创建方法（解决"不是成员"报错）
        static std::unique_ptr<Processor> createProcessor(ProcessorType type);

    private:
        // 私有化构造函数，禁止实例化
        ProcessorFactory() = default;
    };

} // namespace radar

#endif // PROCESSING_PROCESSORFACTORY_H