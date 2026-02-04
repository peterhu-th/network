#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QObject>

#include "../src/audio/AudioSourceFactory.h"
#include "../src/core/Types.h"

using namespace radar;
using namespace radar::audio;

int main(int argc, char *argv[]) {
    //初始化应用
    QCoreApplication app(argc, argv);
    qDebug() << "========================================";
    qDebug() << "   Audio Source Integration Test";
    qDebug() << "========================================";

    //创建音频源
    qDebug() << "Creating AudioSource via Factory...";
    Result<IAudioSource*> factoryResult = AudioSourceFactory::createAudioSource(&app);
    if (!factoryResult.isOk()) {
        qCritical() << "ERROR: Factory failed to create source:" << factoryResult.errorMessage();
        return -1;
    }
    IAudioSource* source = factoryResult.value();
    qDebug() << "SUCCESS: AudioSource created. Type:" << typeid(*source).name();

    //连接信号槽
    QObject::connect(source, &IAudioSource::frameReady, [](const AudioFrame& frame) {
        static int count = 0;
        count++;
        if (count % 10 == 0) {
            qDebug() << "[Data] Frame #" << count
                     << "| Time:" << frame.timestamp
                     << "| Size:" << frame.data.size() << "bytes"
                     << "| Rate:" << frame.sampleRate;
        }
    });
    QObject::connect(source, &IAudioSource::errorOccurred, [](const QString& msg, ErrorCode code) {
        qCritical() << "ERROR SIGNAL RECEIVED:" << msg << "Code:" << (int)code;
    });

    //启动音频采集
    qDebug() << "Starting Audio Capture...";
    Result<void> startResult = source->start();
    if (!startResult.isOk()) {
        qCritical() << "ERROR: Failed to start:" << startResult.errorMessage();
        return -1;
    }
    qDebug() << "SUCCESS: Capture started. Please speak into the microphone.";

    //定时器：5秒后自动停止并退出
    QTimer::singleShot(5000, [source, &app]() {
        qDebug() << "\n[Step 3] Stopping Capture (5 seconds reached)...";
        qDebug() << "Final Duration:" << source->getDuration() << "ms";

        Result<void> stopResult = source->stop();
        if (stopResult.isOk()) {
            qDebug() << "SUCCESS: Stopped gracefully.";
        } else {
            qCritical() << "ERROR: Failed to stop:" << stopResult.errorMessage();
        }
        app.quit();
    });

    //进入事件循环
    return app.exec();
}