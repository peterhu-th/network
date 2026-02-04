#ifndef I_AUDIO_SOURCE_H
#define I_AUDIO_SOURCE_H

#include "../core/Types.h"
#include <QObject>

//audio部分全部使用radar::audio命名空间
//类之间的派生关系：QObject->IAudioSource->AudioSourceBase->SystemAudioSource
//IAudioSource：抽象类作为接口

namespace radar::audio {
    class IAudioSource: public QObject {
        Q_OBJECT

    public:
        explicit IAudioSource(QObject *parent = nullptr): QObject(parent) {}
        ~IAudioSource() override = default;

        virtual Result<void> start() = 0;
        virtual Result<void> stop() = 0;
        //每秒采集次数
        virtual int getSampleRate() = 0;
        //通道数（单声道/双声道）
        virtual uint16_t getChannels() = 0;
        //位深（采样精度）
        virtual uint16_t getSampleSize() = 0;
        //计算采集（从start到stop）的时长
        virtual uint64_t getDuration() = 0;

    signals:
        //采用push方式传递AudioFrame
        void frameReady(const radar::AudioFrame& frame);
        void errorOccurred(const QString& message, radar::ErrorCode error);

    };
}
#endif
