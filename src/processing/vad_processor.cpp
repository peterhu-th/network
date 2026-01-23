#include "vad_processor.h"
#include "radar_utils.h"

namespace radar {

    Result<ProcessedData> VADProcessor::doProcess(const AudioFrame& input) {
        ProcessedData output;
        output.originalFrame = input;
        output.features.clear();

        output.signalStrength = calculateEnergy(input);
        output.isValid = (output.signalStrength > energy_threshold_);

        return Result<ProcessedData>::ok(output);
    }

}