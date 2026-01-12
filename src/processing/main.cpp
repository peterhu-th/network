#include "radar_processing.h"
#include <iostream>

int main() {
    // 1. 构造测试音频帧（16bit、单声道、44100Hz）
    radar::AudioFrame test_frame;
    test_frame.timestamp = 1736677000000; // 示例时间戳
    test_frame.sample_rate = 44100;
    test_frame.channels = 1;
    test_frame.bit_depth = 16;

    // 构造测试PCM数据（模拟有噪声的音频）
    QVector<int16_t> test_pcm;
    for (int i = 0; i < 1024; ++i) {
        test_pcm.push_back(static_cast<int16_t>(sin(i * 0.1) * 1000 + rand() % 100)); // 信号+噪声
    }
    test_frame.data = QByteArray(
        reinterpret_cast<const char*>(test_pcm.constData()),
        test_pcm.size() * sizeof(int16_t)
    );

    // 2. 创建流水线并添加处理器
    radar::PipelineManager pipeline;
    pipeline.addProcessor(std::make_unique<radar::DenoiseProcessor>());
    pipeline.addProcessor(std::make_unique<radar::VADProcessor>(800.0)); // 阈值800
    pipeline.addProcessor(std::make_unique<radar::FeatureExtractor>());

    // 3. 执行处理
    auto result = pipeline.execute(test_frame);

    // 4. 输出结果
    if (result.isOk()) {
        auto processed_data = result.value();
        std::cout << "处理成功！" << std::endl;
        std::cout << "是否为有效信号：" << (processed_data.is_valid ? "是" : "否") << std::endl;
        std::cout << "信号强度：" << processed_data.signal_strength << std::endl;
        std::cout << "最大振幅：" << processed_data.features["max_amplitude"] << std::endl;
        std::cout << "信噪比：" << processed_data.features["snr"] << " dB" << std::endl;
    } else {
        std::cerr << "处理失败：" << result.errorMsg() 
                  << "（错误码：" << static_cast<int>(result.errorCode()) << "）" << std::endl;
    }

    return 0;
}