#include "radar_types.h"
#include "AudioProcessingService.h"
#include "SimulatedPcmSource.h"
#include <iostream>
#include <thread>

void audioProcessingTest() {
    try {
        radar::SimulatedPcmSource pcm_source(radar::SimulatedPcmType::NOISY_VOICE);
        radar::AudioProcessingService processing_service;

        std::cout << "Audio processing test start..." << std::endl;
        for (int i = 1; i <= 10; ++i) {
            radar::AudioFrame frame = pcm_source.generateFrame();
            radar::Result<radar::ProcessedData> result = processing_service.processAudioFrame(frame);

            if (result.isOk()) {
                auto data = result.value();
                std::cout << "Frame " << i << ": valid=" << (data.isValid ? "true" : "false")
                          << ", SNR=" << data.features["snr"].toDouble() << std::endl;
            } else {
                std::cerr << "Frame " << i << " process failed: " << result.errorMsg().toStdString() << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } catch (const std::exception& e) {
        std::cerr << "Test exception: " << e.what() << std::endl;
    }
}

// 自测入口函数（命名直观）
int test() {
    // 本地自测时取消注释，提交时保持注释
    // audioProcessingTest();
    return 0;
}

// 空main函数（兼容编译成库的要求）
int main() {
    return 0;
}