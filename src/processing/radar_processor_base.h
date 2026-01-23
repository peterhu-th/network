#ifndef RADAR_PROCESSOR_BASE_H
#define RADAR_PROCESSOR_BASE_H

#include "radar_types.h"
#include <vector>

namespace radar {

    // 第一层：纯虚抽象类
    class Processor {
    public:
        virtual ~Processor() = default;
        virtual Result<ProcessedData> process(const AudioFrame& input) = 0;
    };

    // 第二层：音频处理器通用基类
    class AudioProcessorBase : public Processor {
    public:
        Result<ProcessedData> process(const AudioFrame& input) override final;

    protected:
        virtual Result<ProcessedData> doProcess(const AudioFrame& input) = 0;

        // 通用辅助方法
        Result<void> validateInput(const AudioFrame& input) {
            if (input.data.isEmpty()) {
                return Result<void>::error("Input audio data is empty", ErrorCode::InvalidData);
            }
            if (input.bitsPerSample != 16) {
                return Result<void>::error("Only 16bit PCM is supported", ErrorCode::InvalidData);
            }
            if (input.data.size() % sizeof(int16_t) != 0) {
                return Result<void>::error("PCM data size mismatch with 16bit depth", ErrorCode::InvalidData);
            }
            return Result<void>::ok();
        }

        std::vector<int16_t> convertToInt16(const AudioFrame& frame) {
            const int16_t* raw_data = reinterpret_cast<const int16_t*>(frame.data.constData());
            int sample_count = frame.data.size() / sizeof(int16_t);
            return std::vector<int16_t>(raw_data, raw_data + sample_count);
        }

        void writeInt16ToFrame(const std::vector<int16_t>& pcm_data, AudioFrame& frame) {
            frame.data = QByteArray(reinterpret_cast<const char*>(pcm_data.data()),
                                    pcm_data.size() * sizeof(int16_t));
        }

        int getSampleCount(const AudioFrame& frame) {
            return frame.data.isEmpty() ? 0 : frame.data.size() / sizeof(int16_t);
        }
    };

} // namespace radar

#endif // RADAR_PROCESSOR_BASE_H