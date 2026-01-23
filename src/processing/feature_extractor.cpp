#include "feature_extractor.h"
#include "radar_utils.h"
#include <cmath>
#include <algorithm>

namespace radar {

    Result<ProcessedData> FeatureExtractor::doProcess(const AudioFrame& input) {
        ProcessedData output;
        output.originalFrame = input;
        output.isValid = true;
        output.signalStrength = calculateEnergy(input);

        output.features["max_amplitude"] = calculateMaxAmplitude(input);
        output.features["snr"] = calculateSNR(input);

        return Result<ProcessedData>::ok(output);
    }

    int16_t FeatureExtractor::calculateMaxAmplitude(const AudioFrame& frame) {
        int sample_count = getSampleCount(frame);
        const int16_t* data = reinterpret_cast<const int16_t*>(frame.data.constData());
        int16_t max_amp = 0;
        for (int i = 0; i < sample_count; ++i) {
            max_amp = std::max(max_amp, static_cast<int16_t>(std::abs(data[i])));
        }
        return max_amp;
    }

    double FeatureExtractor::calculateSNR(const AudioFrame& frame) {
        double signal_energy = calculateEnergy(frame);
        double noise_energy = 100.0;
        return 10 * log10(signal_energy / (noise_energy + 1e-6));
    }

}