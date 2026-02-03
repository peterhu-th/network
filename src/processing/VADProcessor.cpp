#include "VADProcessor.h"
#include <cmath>

namespace radar {

    Result<ProcessedData> VADProcessor::doProcess(const AudioFrame& input) {
        ProcessedData output;
        output.originalFrame = input;

        std::vector<int16_t> pcm_data = convertToInt16(input);
        double rms = calculateRMS(pcm_data);

        output.isValid = (rms > threshold_);
        output.signalStrength = rms / threshold_;
        output.features["rms"] = rms;
        output.features["vad_threshold"] = threshold_;

        return Result<ProcessedData>::ok(output);
    }

    double VADProcessor::calculateRMS(const std::vector<int16_t>& data) {
        double sum = 0.0;
        for (int16_t sample : data) {
            sum += sample * sample;
        }
        double mean = sum / data.size();
        return sqrt(mean);
    }

} // namespace radar