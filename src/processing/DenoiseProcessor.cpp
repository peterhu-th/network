#include "../include/DenoiseProcessor.h"
#include <algorithm>

namespace radar {

    DenoiseProcessor::DenoiseProcessor(const DenoiseConfig& config)
        : m_config(config) {
        m_fft = std::make_unique<kissfft<double>>(config.fftSize, false);
        m_ifft = std::make_unique<kissfft<double>>(config.fftSize, true);
    }

    Result<ProcessedData> DenoiseProcessor::process(const AudioFrame& frame) {
        auto validateRes = validateInput(frame);
        if (!validateRes.isOk()) {
            return Result<ProcessedData>::error("去噪处理器：输入无效", ErrorCode::ProcessingFailed);
        }

        ProcessedData output;

        AudioFrame filteredFrame = applyBandpassFilter(frame);

        const auto* rawData = reinterpret_cast<const int16_t*>(filteredFrame.data.constData());
        int numSamples = filteredFrame.data.size() / sizeof(int16_t);
        int channels = filteredFrame.channels;
        int numFrames = numSamples / channels;

        std::vector<int16_t> aleSamples(numSamples);
        for (int ch = 0; ch < channels; ++ch) {
            std::vector<double> inputDouble(numFrames);
            for (int i = 0; i < numFrames; ++i) {
                inputDouble[i] = static_cast<double>(rawData[i * channels + ch]) / 32768.0;
            }

            std::vector<double> aleOutput1 = applyALE(inputDouble, 1);
            std::vector<double> aleOutput = applyALE(aleOutput1, 2);

            for (int i = 0; i < numFrames; ++i) {
                aleSamples[i * channels + ch] = static_cast<int16_t>(
                    std::max(-1.0, std::min(1.0, aleOutput[i])) * 32767.0);
            }
        }

        output.originalFrame.data = QByteArray(reinterpret_cast<const char*>(aleSamples.data()),
                                               numSamples * sizeof(int16_t));
        output.originalFrame.sampleRate = filteredFrame.sampleRate;
        output.originalFrame.channels = filteredFrame.channels;
        output.originalFrame.sampleSize = filteredFrame.sampleSize;

        output.isValid = true;
        output.signalStrength = 0.92;
        output.features.insert("filter_type", "bandpass");
        output.features.insert("low_cutoff", m_config.lowCutoff);
        output.features.insert("high_cutoff", m_config.highCutoff);
        output.features.insert("sample_rate", m_config.fftSize);

        return Result<ProcessedData>::ok(output);
    }

    std::vector<double> DenoiseProcessor::applyALE(const std::vector<double>& x, int stage) {
        int N = x.size();

        double mu = (stage == 1) ? m_config.aleMu1 : m_config.aleMu2;
        int M = (stage == 1) ? m_config.aleM1 : m_config.aleM2;
        int delta = (stage == 1) ? m_config.aleDelta1 : m_config.aleDelta2;

        std::vector<double> w(M, 0.0);
        std::vector<double> y(N, 0.0);

        auto isSignalActive = [&](int start, int end) {
            if (start < 0) start = 0;
            if (end > (int)x.size()) end = (int)x.size();
            double power = 0.0;
            for (int i = start; i < end; ++i) {
                power += x[i] * x[i];
            }
            power /= (end - start);
            return power > m_config.sadThreshold;
        };

        const int init_protect = 1000;

        for (int n = M + delta; n < N; ++n) {
            double x_vec[M];
            for (int i = 0; i < M; ++i) {
                x_vec[i] = x[n - M - delta + i];
            }

            double pred = 0.0;
            for (int i = 0; i < M; ++i) {
                pred += w[i] * x_vec[i];
            }
            y[n] = pred;

            double e = x[n] - pred;

            if (n > init_protect && !isSignalActive(n - 100, n)) {
                for (int i = 0; i < M; ++i) {
                    w[i] += 2.0 * mu * e * x_vec[i];
                }
            }
        }

        for (int i = 0; i < N; ++i) {
            y[i] = m_config.aleMixRatio * y[i] * m_config.aleSignalGain + (1.0 - m_config.aleMixRatio) * x[i];
        }

        return y;
    }

    AudioFrame DenoiseProcessor::applyBandpassFilter(const AudioFrame& frame) {
        AudioFrame filteredFrame = frame;

        const auto* rawData = reinterpret_cast<const int16_t*>(frame.data.constData());
        int numSamples = frame.data.size() / sizeof(int16_t);
        int channels = frame.channels;
        int numFrames = numSamples / channels;
        int sampleRate = frame.sampleRate;

        std::vector<int16_t> sampleVector(rawData, rawData + numSamples);

        int fftSize = m_config.fftSize;
        int hopSize = m_config.hopSize;
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
                        double HannWindow = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (fftSize - 1)));
                        fftBuffer[i] = std::complex<double>(static_cast<double>(sample) / 32768.0 * HannWindow, 0.0);
                    } else {
                        fftBuffer[i] = std::complex<double>(0.0, 0.0);
                    }
                }

                applyFFT(fftBuffer);

                double binSize = static_cast<double>(sampleRate) / fftSize;
                int lowBin = static_cast<int>(m_config.lowCutoff / binSize);
                int highBin = static_cast<int>(m_config.highCutoff / binSize);
                if (highBin > fftSize / 2) highBin = fftSize / 2;

                for (int i = 0; i < fftSize; ++i) {
                    if (i < lowBin || i > highBin) {
                        fftBuffer[i] = std::complex<double>(0.0, 0.0);
                    }
                }

                applyIFFT(fftBuffer);

                for (int i = 0; i < fftSize; ++i) {
                    int sampleIdx = startIdx + i;
                    if (sampleIdx < numFrames) {
                        accumBuffer[ch][sampleIdx] += fftBuffer[i].real();
                        weightBuffer[ch][sampleIdx] += 1.0;
                    }
                }
            }
        }

        std::vector<int16_t> outputVector(numSamples);
        for (int ch = 0; ch < channels; ++ch) {
            for (int i = 0; i < numFrames; ++i) {
                double normalized = (weightBuffer[ch][i] > 0.0)
                                    ? accumBuffer[ch][i] / weightBuffer[ch][i]
                                    : 0.0;
                outputVector[i * channels + ch] = static_cast<int16_t>(
                    std::max(-1.0, std::min(1.0, normalized)) * 32767.0);
            }
        }

        filteredFrame.data = QByteArray(reinterpret_cast<const char*>(outputVector.data()),
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

}
