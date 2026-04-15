#ifndef PROCESSING_RADAR_PROCESSOR_BASE_H
#define PROCESSING_RADAR_PROCESSOR_BASE_H

#include "Types.h"

namespace radar {

    class Processor {
    public:
        virtual ~Processor() = default;
        virtual Result<ProcessedData> process(const AudioFrame& frame) = 0;

    protected:
        // 输入验证函数
        Result<bool> validateInput(const AudioFrame& frame) {
            if (frame.data.isEmpty() || frame.sampleRate == 0) {
                return Result<bool>::error("输入音频帧无效", ErrorCode::ProcessingFailed);
            }
            return Result<bool>::ok(true);
        }
    };

} // namespace radar

#endif // PROCESSING_RADAR_PROCESSOR_BASE_H
