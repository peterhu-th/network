#ifndef SYSTEM_AUDIO_SOURCE_H
#define SYSTEM_AUDIO_SOURCE_H

#include "AudioSourceBase.h"
#include <QIODevice>
#include <QAudioInput>
#include <QAudioFormat>
#include <QAudioDeviceInfo>

namespace radar::audio {
    class SystemAudioSource: public AudioSourceBase {
        Q_OBJECT
    public:
        explicit SystemAudioSource(QObject *parent = nullptr);
        ~SystemAudioSource() override {
            stop();
        }

        Result<void> start() final;
        Result<void> stop() final;
    //槽函数，用于接收signal
    private slots:
        void readFrame();

    private:
        QAudioInput *m_audioInput;
        QIODevice *m_ioDevice;
        //m_format是数值量，用默认构造函数初始化
        QAudioFormat m_format;
    };
}
#endif
