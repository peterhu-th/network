#ifndef PROCESSING_RADAR_PROCESSOR_BASE_H
#define PROCESSING_RADAR_PROCESSOR_BASE_H

#include "../core/types.h"
#include <memory>

namespace radar {

    class Processor {
    public:
        virtual ~Processor() = default;
        virtual Result<ProcessedData> process(const AudioFrame& frame) = 0;

    protected:
        // 补充validateInput的实现（之前只声明未定义）
        Result<bool> validateInput(const AudioFrame& frame) {
            if (frame.data.isEmpty() || frame.sampleRate == 0) {
                return Result<bool>::error("输入音频帧无效", ErrorCode::ProcessingFailed);
            }
            return Result<bool>::ok(true);
        }
    };

} // namespace radar

#endif // PROCESSING_RADAR_PROCESSOR_BASE_H