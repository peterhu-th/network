#include "pipeline_manager.h"
#include "denoise_processor.h"
#include "vad_processor.h"
#include "feature_extractor.h"
#include "audio_module_api.h"

#include <QtCore/QCoreApplication>
#include <iostream>
#include <thread>

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    // 初始化处理流水线
    radar::PipelineManager pipeline;
    pipeline.addProcessor(std::make_unique<radar::DenoiseProcessor>());
    pipeline.addProcessor(std::make_unique<radar::VADProcessor>(800.0));
    pipeline.addProcessor(std::make_unique<radar::FeatureExtractor>());

    std::cout << "音频处理模块启动，开始对接Audio模块..." << std::endl;

    // 循环读取Audio模块数据并处理
    int frameIndex = 0;
    while (true) {
        try {
            // 调用Audio模块真实接口读取数据
            radar::AudioFrame audioFrame = audio_module::readAudioFrame();
            frameIndex++;

            // 空数据校验
            if (audioFrame.data.isEmpty()) {
                std::cerr << "警告：第" << frameIndex << "帧数据为空，跳过处理" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            // 执行流水线处理
            auto processResult = pipeline.execute(audioFrame);

            // 处理结果输出
            if (processResult.isOk()) {
                radar::ProcessedData result = processResult.value();
                std::cout << "\n第 " << frameIndex << " 帧处理完成：" << std::endl;
                std::cout << "  数据源：" << result.originalFrame.sourceId.toStdString() << std::endl;
                std::cout << "  有效信号：" << (result.isValid ? "是" : "否") << std::endl;
                std::cout << "  信号强度：" << result.signalStrength << std::endl;
                std::cout << "  最大振幅：" << result.features["max_amplitude"].toInt() << std::endl;
            } else {
                std::cerr << "第" << frameIndex << "帧处理失败：" << processResult.errorMsg().toStdString() << std::endl;
            }

        } catch (const std::exception& e) {
            std::cerr << "读取Audio模块数据异常：" << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    return a.exec();
}