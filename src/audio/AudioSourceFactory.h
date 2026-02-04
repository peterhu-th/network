#ifndef AUDIO_SOURCE_FACTORY_H
#define AUDIO_SOURCE_FACTORY_H

#include "IAudioSource.h"

namespace radar::audio {
    class AudioSourceFactory: public QObject {
        Q_OBJECT
    public:
        static Result<IAudioSource *> createAudioSource(QObject *parent);
    private:
        AudioSourceFactory() {}
        ~AudioSourceFactory() override = default;
    };

}
#endif
