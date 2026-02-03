#include "DenoiseProcessor.h"

namespace radar {

    Result<ProcessedData> DenoiseProcessor::doProcess(const AudioFrame& input) {
        ProcessedData output;
        output.originalFrame = input;

        std::vector<int16_t> pcm_data = convertToInt16(input);
        std::vector<int16_t> denoised_data = meanFilter(pcm_data);

        writeInt16ToFrame(denoised_data, output.originalFrame);
        output.isValid = true;
        output.signalStrength = 1.0;
        output.features["denoised"] = true;

        return Result<ProcessedData>::ok(output);
    }

    std::vector<int16_t> DenoiseProcessor::meanFilter(const std::vector<int16_t>& data) {
        std::vector<int16_t> filtered(data.size());
        int window_size = 3;
        for (int i = 0; i < data.size(); ++i) {
            int sum = 0, count = 0;
            for (int j = -window_size/2; j <= window_size/2; ++j) {
                if (i + j >= 0 && i + j < data.size()) {
                    sum += data[i + j];
                    count++;
                }
            }
            filtered[i] = static_cast<int16_t>(sum / count);
        }
        return filtered;
    }

} // namespace radar