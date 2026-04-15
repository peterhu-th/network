#include "../include/DenoiseProcessor.h"
#include <algorithm>

// 带通滤波处理器 - 简化版（仅保留滤波功能）
//
// 处理流程：
// 输入音频 → STFT（加窗）→ FFT（时域→频域）→ 带通滤波（保留2000-19000Hz）→ IFFT（频域→时域）→ 重叠相加 → 输出音频
//
// 关键参数：
// - FFT窗口大小：8192（频率分辨率高）
// - 帧移：2048（75%重叠，时域平滑）
// - 带通范围：2000-19000 Hz（去除低频和高频噪声）

namespace radar {

    // 构造函数：初始化带通滤波参数和FFT工具
    // 参数：
    //   lowCutoff - 低频截止频率（Hz），默认2000Hz
    //   highCutoff - 高频截止频率（Hz），默认19000Hz
    //   sampleRate - 采样率（Hz），默认44100Hz
    DenoiseProcessor::DenoiseProcessor(double lowCutoff, double highCutoff, int sampleRate)
        : m_lowCutoff(lowCutoff)
        , m_highCutoff(highCutoff)
        , m_sampleRate(sampleRate)
        , m_fftSize(2048) {
        // 创建FFT（前向）和IFFT（逆向）工具对象
        m_fft = std::make_unique<kissfft<double>>(m_fftSize, false);
        m_ifft = std::make_unique<kissfft<double>>(m_fftSize, true);
    }

    // 主处理函数：处理音频帧
    // 输入：AudioFrame（原始音频）
    // 输出：ProcessedData（处理后的音频+元数据）
    Result<ProcessedData> DenoiseProcessor::process(const AudioFrame& frame) {
        // 步骤1：验证输入有效性
        auto validateRes = validateInput(frame);
        if (!validateRes.isOk()) {
            return Result<ProcessedData>::error("去噪处理器：输入无效", ErrorCode::ProcessingFailed);
        }

        // 步骤2：创建输出数据结构
        ProcessedData output;
        
        // 步骤3：应用带通滤波（去除低频和高频噪声）
        AudioFrame filteredFrame = applyBandpassFilter(frame);
        
        // 步骤4：应用单级ALE（温和参数，保留正常信号）
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
            
            // 两级ALE：先弱后强
            std::vector<double> aleOutput1 = applyALE(inputDouble, 0.0005, 16, 3);
            std::vector<double> aleOutput = applyALE(aleOutput1, 0.001, 32, 5);
            
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
        
        // 步骤5：设置输出元数据
        output.isValid = true;
        output.signalStrength = 0.92;
        output.features.insert("filter_type", "bandpass");
        output.features.insert("low_cutoff", m_lowCutoff);
        output.features.insert("high_cutoff", m_highCutoff);
        output.features.insert("sample_rate", m_sampleRate);

        // 步骤6：返回处理结果
        return Result<ProcessedData>::ok(output);
    }

    // 自适应线谱增强器（ALE）- LMS算法，带SAD信号检测和初始收敛保护
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

            // 只有过了初始收敛阶段且是纯噪声段，才更新权重
            if (n > init_protect && !isSignalActive(n - 100, n)) {
                for (int i = 0; i < M; ++i) {
                    w[i] += 2.0 * mu * e * x_vec[i];
                }
            }
        }

        // 混合输出：70% ALE输出(增强1.3倍) + 30% 原始信号
        const double signalGain = 1.3;
        for (int i = 0; i < N; ++i) {
            y[i] = 0.7 * y[i] * signalGain + 0.3 * x[i];
        }

        return y;
    }

    // 带通滤波核心函数：使用STFT方法处理音频
    // 原理：短时傅里叶变换（STFT）+ 频域滤波 + 逆短时傅里叶变换（ISTFT）
    AudioFrame DenoiseProcessor::applyBandpassFilter(const AudioFrame& frame) {
        // ==================== 阶段1：准备数据 ====================
        AudioFrame filteredFrame = frame;
        
        // 从QByteArray提取int16样本
        const auto* rawData = reinterpret_cast<const int16_t*>(frame.data.constData());
        int numSamples = frame.data.size() / sizeof(int16_t);
        int channels = frame.channels;
        int numFrames = numSamples / channels;

        // 转换为向量便于处理
        std::vector<int16_t> sampleVector(rawData, rawData + numSamples);

        // ==================== 阶段2：STFT参数设置 ====================
        const int fftSize = 8192;      // FFT窗口大小（频率分辨率高）
        const int hopSize = fftSize / 4;  // 帧移量（75%重叠，时域平滑）
        int numFFTFrames = (numFrames - fftSize) / hopSize + 1;
        if (numFFTFrames <= 0) numFFTFrames = 1;

        // 累加缓冲区和权重缓冲区（用于重叠相加）
        std::vector<std::vector<double>> accumBuffer(channels, std::vector<double>(numFrames, 0.0));
        std::vector<std::vector<double>> weightBuffer(channels, std::vector<double>(numFrames, 0.0));

        // ==================== 阶段3：分帧处理（STFT + 滤波 + ISTFT ====================
        for (int ch = 0; ch < channels; ++ch) {
            for (int frameIdx = 0; frameIdx < numFFTFrames; ++frameIdx) {
                int startIdx = frameIdx * hopSize;

                // ---------- 子步骤3.1：加窗 & 归一化 ----------
                std::vector<std::complex<double>> fftBuffer(fftSize);
                for (int i = 0; i < fftSize; ++i) {
                    int sampleIdx = startIdx + i;
                    if (sampleIdx < numFrames) {
                        int16_t sample = sampleVector[sampleIdx * channels + ch];
                        
                        // 汉宁窗（Hann Window）- 减少频谱泄漏
                        // 公式：w(n) = 0.5 * (1 - cos(2πn/(N-1)))
                        double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (fftSize - 1)));
                        
                        // 归一化到[-1, 1]范围（int16 → double）
                        fftBuffer[i] = static_cast<double>(sample) * window / 32768.0;
                    } else {
                        fftBuffer[i] = 0.0;
                    }
                }

                // ---------- 子步骤3.2：FFT - 时域 → 频域 ----------
                applyFFT(fftBuffer);

                // ---------- 子步骤3.3：频域滤波（核心！）----------
                double binWidth = static_cast<double>(m_sampleRate) / fftSize;

                for (int i = 0; i < fftSize; ++i) {
                    // 计算当前FFT bin对应的频率
                    double freq = i * binWidth;
                    
                    // 处理负频率（FFT输出对称）
                    if (i >= fftSize / 2) {
                        freq = (i - fftSize) * binWidth;
                    }
                    
                    // 带通滤波：只保留2000~19000 Hz
                    // 频率 < 2000 Hz 或 > 19000 Hz → 置0（去除噪声）
                    // 2000~19000 Hz → 保留（目标信号）
                    if (freq < m_lowCutoff || freq > m_highCutoff) {
                        fftBuffer[i] = 0.0;
                    }
                }

                // ---------- 子步骤3.4：IFFT - 频域 → 时域 ----------
                applyIFFT(fftBuffer);

                // ---------- 子步骤3.5：重叠相加（OLA）----------
                for (int i = 0; i < fftSize; ++i) {
                    int sampleIdx = startIdx + i;
                    if (sampleIdx < numFrames) {
                        // 再次加窗（合成窗）
                        double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (fftSize - 1)));
                        
                        // 累加多帧结果
                        accumBuffer[ch][sampleIdx] += fftBuffer[i].real() * window;
                        weightBuffer[ch][sampleIdx] += window * window;
                    }
                }
            }
        }

        // ==================== 阶段4：归一化 & 输出 ====================
        std::vector<int16_t> outputSamples(numSamples);
        for (int ch = 0; ch < channels; ++ch) {
            for (int i = 0; i < numFrames; ++i) {
                double sample = 0.0;
                
                // 权重归一化：修正重叠相加带来的幅度变化
                if (weightBuffer[ch][i] > 0.0) {
                    sample = accumBuffer[ch][i] / weightBuffer[ch][i];
                }
                
                // 防溢出裁剪：确保样本在[-1, 1]范围内
                sample = std::max(-1.0, std::min(1.0, sample));
                
                // 转回int16格式（double → int16）
                outputSamples[i * channels + ch] = static_cast<int16_t>(sample * 32767.0);
            }
        }

        // ==================== 阶段5：封装输出 ====================
        filteredFrame.data = QByteArray(reinterpret_cast<const char*>(outputSamples.data()),
                                       numSamples * sizeof(int16_t));

        return filteredFrame;
    }

    // 前向FFT：时域 → 频域
    void DenoiseProcessor::applyFFT(std::vector<std::complex<double>>& data) {
        kissfft<double> fft(data.size(), false);  // false = 前向FFT
        std::vector<std::complex<double>> output(data.size());
        fft.transform(data.data(), output.data());
        data = std::move(output);
    }

    // 逆FFT：频域 → 时域
    void DenoiseProcessor::applyIFFT(std::vector<std::complex<double>>& data) {
        kissfft<double> ifft(data.size(), true);  // true = 逆FFT
        std::vector<std::complex<double>> output(data.size());
        ifft.transform(data.data(), output.data());
        
        // IFFT归一化：除以FFT大小（IFFT输出会放大N倍）
        double scale = 1.0 / data.size();
        for (auto& val : output) {
            val *= scale;
        }
        data = std::move(output);
    }

} // namespace radar