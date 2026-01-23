#include "radar_utils.h"
#include <cmath>

namespace radar {

    double calculateEnergy(const AudioFrame& frame) {
        if (frame.data.isEmpty() || frame.bitsPerSample != 16) {
            return 0.0;
        }
        const int16_t* data = reinterpret_cast<const int16_t*>(frame.data.constData());
        int sample_count = frame.data.size() / sizeof(int16_t);
        double sum = 0.0;
        for (int i = 0; i < sample_count; ++i) {
            sum += static_cast<double>(data[i]) * data[i];
        }
        return sum / sample_count;
    }

}