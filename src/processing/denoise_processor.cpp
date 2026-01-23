#include "denoise_processor.h"
#include <algorithm>

namespace radar {

    Result<ProcessedData> DenoiseProcessor::doProcess(const AudioFrame& input) {
        ProcessedData output;
        output.originalFrame = input;
        output.isValid = true;
        output.signalStrength = 0.0;
        output.features.clear();

        std::vector<int16_t> pcm_data = convertToInt16(input);
        std::vector<int16_t> denoised_data = meanFilter(pcm_data);
        writeInt16ToFrame(denoised_data, output.originalFrame);

        return Result<ProcessedData>::ok(output);
    }

    std::vector<int16_t> DenoiseProcessor::meanFilter(const std::vector<int16_t>& pcm_data) {
        if (pcm_data.size() <= 2) {
            return pcm_data;
        }
        std::vector<int16_t> result = pcm_data;
        for (int i = 1; i < pcm_data.size() - 1; ++i) {
            result[i] = static_cast<int16_t>((pcm_data[i-1] + pcm_data[i] + pcm_data[i+1]) / 3);
        }
        return result;
    }

}