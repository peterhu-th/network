#include "FeatureExtractor.h"
#include <cmath>
#include <algorithm>

namespace radar {

    Result<ProcessedData> FeatureExtractor::doProcess(const AudioFrame& input) {
        ProcessedData output;
        output.originalFrame = input;

        std::vector<int16_t> pcm_data = convertToInt16(input);
        double snr = calculateSNR(pcm_data);
        int16_t max_amp = calculateMaxAmplitude(pcm_data);

        output.isValid = true;
        output.signalStrength = snr;
        output.features["snr"] = snr;
        output.features["maxAmplitude"] = max_amp;
        output.features["sampleCount"] = pcm_data.size();

        return Result<ProcessedData>::ok(output);
    }

    double FeatureExtractor::calculateSNR(const std::vector<int16_t>& data) {
        double signal_power = 0.0, noise_power = 0.0;
        double mean = 0.0;
        for (int16_t sample : data) mean += sample;
        mean /= data.size();

        for (int16_t sample : data) {
            signal_power += (sample - mean) * (sample - mean);
            noise_power += mean * mean;
        }
        return noise_power > 0 ? 10 * log10(signal_power / noise_power) : 0.0;
    }

    int16_t FeatureExtractor::calculateMaxAmplitude(const std::vector<int16_t>& data) {
        return *std::max_element(data.begin(), data.end(), [](int16_t a, int16_t b) {
            return abs(a) < abs(b);
        });
    }

} // namespace radar