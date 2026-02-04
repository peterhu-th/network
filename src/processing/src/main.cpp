#include <QCoreApplication>
#include <QDebug>
#include <memory>
#include <QDateTime>
// 核心类型头文件（修正路径）
#include "../../src/core/types.h"
// processing模块头文件
#include "../include/radar_processor_base.h"
#include "../include/DenoiseProcessor.h"
#include "../include/VADProcessor.h"
#include "../include/FeatureExtractor.h"
#include "../include/PipelineManager.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    // 1. 构造模拟音频帧（无需依赖audio模块）
    radar::AudioFrame frame;
    frame.timestamp = QDateTime::currentMSecsSinceEpoch();
    frame.sampleRate = 44100;
    frame.sourceId = "simulated_microphone";
    // 生成模拟PCM数据（16位单声道，0.5秒音频）
    QByteArray simulatedData;
    for (int i = 0; i < 44100 * 1; i++) { // 44100采样率 * 1秒 * 2字节（16位）
        simulatedData.append(static_cast<char>(i % 255));
        simulatedData.append(static_cast<char>((i % 255) >> 8));
    }
    frame.data = simulatedData;

    // 2. 初始化processing流水线
    radar::PipelineManager pipeline;
    pipeline.addProcessor(std::make_unique<radar::DenoiseProcessor>());
    pipeline.addProcessor(std::make_unique<radar::VADProcessor>(0.5));
    pipeline.addProcessor(std::make_unique<radar::FeatureExtractor>());

    // 3. 调试核心处理逻辑（断点打在这里）
    auto result = pipeline.execute(frame);

    // 4. 打印处理结果
    if (result.isOk()) {
        qDebug() << "\n===== 模拟音频处理结果 =====";
        qDebug() << "信号是否有效：" << result.value().isValid;
        qDebug() << "信号强度：" << result.value().signalStrength;
        qDebug() << "提取的特征：" << result.value().features;
    } else {
        qDebug() << "\n===== 处理失败 =====";
        qDebug() << "错误信息：" << result.errorMessage();
        qDebug() << "错误码：" << static_cast<int>(result.errorCode());
    }

    return 0;
}