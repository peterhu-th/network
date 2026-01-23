#include "radar_processor_base.h"

namespace radar {

    Result<ProcessedData> AudioProcessorBase::process(const AudioFrame& input) {
        auto validate_result = validateInput(input);
        if (!validate_result.isOk()) {
            return Result<ProcessedData>::error(
                "Input validation failed: " + validate_result.errorMsg(),
                validate_result.errorCode()
            );
        }
        return doProcess(input);
    }

}