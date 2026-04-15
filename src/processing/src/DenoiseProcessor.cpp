#include "../include/DenoiseProcessor.h"
#include <algorithm>
#include <cmath>
#include <QDebug>

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

        std::vector<double> window(fftSize);
        for (int i = 0; i < fftSize; ++i) {
            window[i] = 0.5 * (1 - std::cos(2.0 * M_PI * i / (fftSize - 1)));
        }

        for (int ch = 0; ch < channels; ++ch) {
            std::vector<double> monoInput(numFrames);
            for (int i = 0; i < numFrames; ++i) {
                monoInput[i] = static_cast<double>(sampleVector[i * channels + ch]) / 32768.0;
            }

            std::vector<double> aleOutput = applyALE(monoInput, 0.0005, 16, 3);
            aleOutput = applyALE(aleOutput, 0.001, 32, 5);

            std::vector<std::vector<double>> fftFrames(numFFTFrames, std::vector<double>(fftSize));
            for (int f = 0; f < numFFTFrames; ++f) {
                int startIdx = f * hopSize;
                for (int i = 0; i < fftSize; ++i) {
                    if (startIdx + i < numFrames) {
                        fftFrames[f][i] = aleOutput[startIdx + i] * window[i];
                    } else {
                        fftFrames[f][i] = 0.0;
                    }
                }
            }

            for (int f = 0; f < numFFTFrames; ++f) {
                std::vector<std::complex<double>> frameData(fftSize);
                for (int i = 0; i < fftSize; ++i) {
                    frameData[i] = fftFrames[f][i];
                }

                applyFFT(frameData);

                double binWidth = static_cast<double>(m_sampleRate) / fftSize;
                for (int k = 0; k < fftSize; ++k) {
                    double freq = k * binWidth;
                    if (k > fftSize / 2) {
                        freq = (fftSize - k) * binWidth;
                    }
                    if (freq < m_lowCutoff || freq > m_highCutoff) {
                        frameData[k] = 0.0;
                    }
                }

                applyIFFT(frameData);

                int startIdx = f * hopSize;
                for (int i = 0; i < fftSize; ++i) {
                    if (startIdx + i < numFrames) {
                        accumBuffer[ch][startIdx + i] += frameData[i].real() * window[i];
                        weightBuffer[ch][startIdx + i] += window[i] * window[i];
                    }
                }
            }

            for (int i = 0; i < numFrames; ++i) {
                if (weightBuffer[ch][i] > 0.0) {
                    accumBuffer[ch][i] /= weightBuffer[ch][i];
                }
            }

            for (int i = 0; i < numFrames; ++i) {
                sampleVector[i * channels + ch] = static_cast<int16_t>(
                    std::max(-1.0, std::min(1.0, accumBuffer[ch][i])) * 32767.0);
            }
        }

        filteredFrame.data = QByteArray(reinterpret_cast<const char*>(sampleVector.data()),
                                       numSamples * sizeof(int16_t));

        return filteredFrame;
    }

    std::vector<double> DenoiseProcessor::applyALE(const std::vector<double>& x,
                                                    double mu,
                                                    int M,
                                                    int delta) {
        int N = x.size();
        std::vector<double> w(M, 0.0);
        std::vector<double> y(N, 0.0);

        auto isSignalActive = [&x](int start, int end) {
            if (start < 0) start = 0;
            if (end > (int)x.size()) end = (int)x.size();
            double power = 0.0;
            for (int i = start; i < end; ++i) {
                power += x[i] * x[i];
            }
            power /= (end - start);
            return power > 1e-3;
        };

        const int init_protect = 1000;

        for (int n = M + delta; n < N; ++n) {
            double x_vec[32];
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

        const double signalGain = 1.3;
        for (int i = 0; i < N; ++i) {
            y[i] = 0.7 * y[i] * signalGain + 0.3 * x[i];
        }

        return y;
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
