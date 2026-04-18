#include "../include/AudioProcessingService.h"
#include "Types.h"
#include <QCoreApplication>
#include <QDebug>

using namespace radar;

void testProcessingPipeline() {
    AudioProcessingService processingService;
    AudioFrame testFrame;
    testFrame.timestamp = 1738612345678;
    testFrame.sampleRate = 16000;
    testFrame.channels = 1;
    testFrame.sampleSize = 16;
    testFrame.data = QByteArray(3200, 0);
    testFrame.sourceId = "mic_001";

    auto result = processingService.processAudio(testFrame);
    if (result.isOk()) {
        qDebug() << "[Processing模块测试] 流水线处理完成：";
        qDebug() << "  信号有效性：" << result.value().isValid;
        qDebug() << "  信号强度：" << result.value().signalStrength;
        qDebug() << "  特征数据：" << result.value().features;
    } else {
        qDebug() << "[Processing模块测试] 流水线处理失败";
    }
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    testProcessingPipeline();
    return a.exec();
}
