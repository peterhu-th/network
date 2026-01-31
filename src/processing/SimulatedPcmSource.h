#ifndef RADAR_SIMULATED_PCM_SOURCE_H
#define RADAR_SIMULATED_PCM_SOURCE_H

#include "radar_types.h"
#include <random>
#include <chrono>

namespace radar {

enum class SimulatedPcmType {
    CLEAN_VOICE,
    NOISY_VOICE,
    SILENCE
};

class SimulatedPcmSource {
public:
    SimulatedPcmSource(SimulatedPcmType type = SimulatedPcmType::NOISY_VOICE)
        : pcm_type_(type), frame_index_(0) {
        std::random_device rd;
        rng_.seed(rd());
        noise_dist_ = std::uniform_real_distribution<double>(-500.0, 500.0);
    }

    AudioFrame generateFrame() {
        AudioFrame frame;
        frame.sampleRate = 16000;
        frame.channels = 1;
        frame.bitsPerSample = 16;
        frame.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        frame.sourceId = "simulated_pcm_source_" + QString::number(static_cast<int>(pcm_type_));

        const int sample_count = 1600;
        std::vector<int16_t> pcm_data(sample_count, 0);

        switch (pcm_type_) {
            case SimulatedPcmType::CLEAN_VOICE:
                generateCleanVoice(pcm_data, sample_count);
                break;
            case SimulatedPcmType::NOISY_VOICE:
                generateNoisyVoice(pcm_data, sample_count);
                break;
            case SimulatedPcmType::SILENCE:
                break;
        }

        frame.data = QByteArray(
            reinterpret_cast<const char*>(pcm_data.data()),
            sample_count * sizeof(int16_t)
        );
        frame_index_++;
        return frame;
    }

    void setPcmType(SimulatedPcmType type) {
        pcm_type_ = type;
    }

private:
    SimulatedPcmType pcm_type_;
    int frame_index_;
    std::mt19937 rng_;
    std::uniform_real_distribution<double> noise_dist_;

    void generateCleanVoice(std::vector<int16_t>& pcm_data, int sample_count) {
        const double freq = 440.0;
        const double amplitude = 3000.0;
        for (int i = 0; i < sample_count; i++) {
            double sample = amplitude * sin(2 * M_PI * freq * i / 16000.0);
            pcm_data[i] = static_cast<int16_t>(sample);
        }
    }

    void generateNoisyVoice(std::vector<int16_t>& pcm_data, int sample_count) {
        generateCleanVoice(pcm_data, sample_count);
        for (int i = 0; i < sample_count; i++) {
            double noise = noise_dist_(rng_);
            pcm_data[i] += static_cast<int16_t>(noise);
            pcm_data[i] = std::clamp(pcm_data[i], static_cast<int16_t>(-32767), static_cast<int16_t>(32767));
        }
    }
};

} // namespace radar

#endif // RADAR_SIMULATED_PCM_SOURCE_H