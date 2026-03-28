#include "../include/DenoiseProcessor.h"
#include <algorithm>

// 带通滤波处理器 - 简化版（仅保留滤波功能）

namespace radar {

    DenoiseProcessor::DenoiseProcessor(double lowCutoff, double highCutoff, int sampleRate)
        : m_lowCutoff(lowCutoff)
        , m_highCutoff(highCutoff)
        , m_sampleRate(sampleRate)
        , m_fftSize(2048) {
        m_fft = std::make_unique<kissfft<double>>(m_fftSize, false);
        m_ifft = std::make_unique<kissfft<double>>(m_fftSize, true);
    }

    Result<ProcessedData> DenoiseProcessor::process(const AudioFrame& frame) {
        auto validateRes = validateInput(frame);
        if (!validateRes.isOk()) {
            return Result<ProcessedData>::error("去噪处理器：输入无效", ErrorCode::ProcessingFailed);
        }

        ProcessedData output;
        output.originalFrame = applyBandpassFilter(frame);
        output.isValid = true;
        output.signalStrength = 0.92;
        output.features.insert("filter_type", "bandpass");
        output.features.insert("low_cutoff", m_lowCutoff);
        output.features.insert("high_cutoff", m_highCutoff);
        output.features.insert("sample_rate", m_sampleRate);

        return Result<ProcessedData>::ok(output);
    }

    AudioFrame DenoiseProcessor::applyBandpassFilter(const AudioFrame& frame) {
        AudioFrame filteredFrame = frame;
        const auto* rawData = reinterpret_cast<const int16_t*>(frame.data.constData());
        int numSamples = frame.data.size() / sizeof(int16_t);
        int channels = frame.channels;
        int numFrames = numSamples / channels;

        std::vector<int16_t> sampleVector(rawData, rawData + numSamples);

        const int fftSize = 8192;
        const int hopSize = fftSize / 4;
        int numFFTFrames = (numFrames - fftSize) / hopSize + 1;
        if (numFFTFrames <= 0) numFFTFrames = 1;

        std::vector<std::vector<double>> accumBuffer(channels, std::vector<double>(numFrames, 0.0));
        std::vector<std::vector<double>> weightBuffer(channels, std::vector<double>(numFrames, 0.0));

        for (int ch = 0; ch < channels; ++ch) {
            for (int frameIdx = 0; frameIdx < numFFTFrames; ++frameIdx) {
                int startIdx = frameIdx * hopSize;

                std::vector<std::complex<double>> fftBuffer(fftSize);
                for (int i = 0; i < fftSize; ++i) {
                    int sampleIdx = startIdx + i;
                    if (sampleIdx < numFrames) {
                        int16_t sample = sampleVector[sampleIdx * channels + ch];
                        double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (fftSize - 1)));
                        fftBuffer[i] = static_cast<double>(sample) * window / 32768.0;
                    } else {
                        fftBuffer[i] = 0.0;
                    }
                }

                applyFFT(fftBuffer);

                double binWidth = static_cast<double>(m_sampleRate) / fftSize;

                for (int i = 0; i < fftSize; ++i) {
                    double freq = i * binWidth;
                    if (i >= fftSize / 2) {
                        freq = (i - fftSize) * binWidth;
                    }
                    if (freq < m_lowCutoff || freq > m_highCutoff) {
                        fftBuffer[i] = 0.0;
                    }
                }

                applyIFFT(fftBuffer);

                for (int i = 0; i < fftSize; ++i) {
                    int sampleIdx = startIdx + i;
                    if (sampleIdx < numFrames) {
                        double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (fftSize - 1)));
                        accumBuffer[ch][sampleIdx] += fftBuffer[i].real() * window;
                        weightBuffer[ch][sampleIdx] += window * window;
                    }
                }
            }
        }

        std::vector<int16_t> outputSamples(numSamples);
        for (int ch = 0; ch < channels; ++ch) {
            for (int i = 0; i < numFrames; ++i) {
                double sample = 0.0;
                if (weightBuffer[ch][i] > 0.0) {
                    sample = accumBuffer[ch][i] / weightBuffer[ch][i];
                }
                sample = std::max(-1.0, std::min(1.0, sample));
                outputSamples[i * channels + ch] = static_cast<int16_t>(sample * 32767.0);
            }
        }

        filteredFrame.data = QByteArray(reinterpret_cast<const char*>(outputSamples.data()),
                                       numSamples * sizeof(int16_t));

        return filteredFrame;
    }

    void DenoiseProcessor::applyFFT(std::vector<std::complex<double>>& data) {
        kissfft<double> fft(data.size(), false);
        std::vector<std::complex<double>> output(data.size());
        fft.transform(data.data(), output.data());
        data = std::move(output);
    }

    void DenoiseProcessor::applyIFFT(std::vector<std::complex<double>>& data) {
        kissfft<double> ifft(data.size(), true);
        std::vector<std::complex<double>> output(data.size());
        ifft.transform(data.data(), output.data());
        double scale = 1.0 / data.size();
        for (auto& val : output) {
            val *= scale;
        }
        data = std::move(output);
    }

} // namespace radar