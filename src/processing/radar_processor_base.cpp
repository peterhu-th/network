#include "radar_processor_base.h"
#include <cstring>

namespace radar {

    std::vector<int16_t> AudioProcessorBase::convertToInt16(const AudioFrame& frame) {
        const int16_t* raw_data = reinterpret_cast<const int16_t*>(frame.data.constData());
        int sample_count = frame.data.size() / sizeof(int16_t);
        return std::vector<int16_t>(raw_data, raw_data + sample_count);
    }

    void AudioProcessorBase::writeInt16ToFrame(const std::vector<int16_t>& pcm_data, AudioFrame& frame) {
        frame.data = QByteArray(reinterpret_cast<const char*>(pcm_data.data()), pcm_data.size() * sizeof(int16_t));
    }

    Result<void> AudioProcessorBase::validateInput(const AudioFrame& input) {
        if (input.data.isEmpty()) {
            return Result<void>::error("Input audio frame data is empty", ErrorCode::InvalidData);
        }
        if (input.bitsPerSample != 16) {
            return Result<void>::error("Only 16bit PCM is supported", ErrorCode::InvalidData);
        }
        return Result<void>::ok();
    }

} // namespace radar