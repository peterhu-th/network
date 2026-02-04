#include "../include/AudioProcessingService.h"
#include "../../core/types.h"
#include <QCoreApplication>
#include <QDebug>

using namespace radar;

void testProcessingPipeline() {
    AudioProcessingService defaultService;
    AudioFrame testFrame;
    testFrame.timestamp = 1738612345678;
    testFrame.sampleRate = 16000;
    testFrame.channels = 1;
    testFrame.bitsPerSample = 16;
    testFrame.data = QByteArray(3200, 0);
    testFrame.sourceId = "mic_001";

    auto result = defaultService.processAudioFrame(testFrame);
    if (result.isOk()) {
        qDebug() << "[Processing模块测试] 标准流水线处理完成：";
        qDebug() << "  信号有效性：" << result.value().isValid;
        qDebug() << "  信号强度：" << result.value().signalStrength;
        qDebug() << "  特征数据：" << result.value().features;
    } else {
        qDebug() << "[Processing模块测试] 标准流水线处理失败";
    }

    std::vector<std::unique_ptr<Processor>> customProcessors;
    customProcessors.push_back(ProcessorFactory::createDenoiseProcessor());
    customProcessors.push_back(ProcessorFactory::createVADProcessor(900.0));
    auto customService = AudioProcessingService::createCustomService(std::move(customProcessors));
    auto customResult = customService->processAudioFrame(testFrame);
    if (customResult.isOk()) {
        qDebug() << "[Processing模块测试] 自定义流水线处理完成：";
        qDebug() << "  信号有效性：" << customResult.value().isValid;
        qDebug() << "  信号强度：" << customResult.value().signalStrength;
    } else {
        qDebug() << "[Processing模块测试] 自定义流水线处理失败";
    }
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    testProcessingPipeline();
    return a.exec();
}