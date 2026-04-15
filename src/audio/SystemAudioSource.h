#ifndef SYSTEM_AUDIO_SOURCE_H
#define SYSTEM_AUDIO_SOURCE_H

#include "AudioSourceBase.h"
#include <QIODevice>
#include <QAudioFormat>
#include <QAudioSource>
#include <QAudioDevice>

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
        QAudioSource *m_audioSource;
        QIODevice *m_ioDevice;
        QAudioDevice m_audioDevice;
        QAudioFormat m_format;
    };
}
#endif
