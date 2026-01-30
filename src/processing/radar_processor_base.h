#ifndef RADAR_PROCESSOR_BASE_H
#define RADAR_PROCESSOR_BASE_H

#include "radar_types.h"
#include <vector>

namespace radar {

    class Processor {
    public:
        virtual ~Processor() = default;
        virtual Result<ProcessedData> process(const AudioFrame& input) = 0;
    };

    class AudioProcessorBase : public Processor {
    public:
        Result<ProcessedData> process(const AudioFrame& input) override final {
            auto validate_result = validateInput(input);
            if (!validate_result.isOk()) {
                return Result<ProcessedData>::error(validate_result.errorMsg(), validate_result.errorCode());
            }
            return doProcess(input);
        }

    protected:
        virtual Result<ProcessedData> doProcess(const AudioFrame& input) = 0;
        std::vector<int16_t> convertToInt16(const AudioFrame& frame);
        void writeInt16ToFrame(const std::vector<int16_t>& pcm_data, AudioFrame& frame);
        Result<void> validateInput(const AudioFrame& input);
    };

} // namespace radar

#endif // RADAR_PROCESSOR_BASE_H