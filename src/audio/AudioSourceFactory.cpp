#include "AudioSourceFactory.h"
#include "SystemAudioSource.h"
#include "../core/Config.h"

namespace radar::audio {
    Result<IAudioSource *> AudioSourceFactory::createAudioSource(QObject *parent) {
        auto& config = radar::Config::instance();
        QString type = config.audioConfig().value("type", "system").toString();

        if (type == "system") {
            IAudioSource *source = new SystemAudioSource(parent);
            return Result<IAudioSource *>::ok(source);
        } else {
            return Result<IAudioSource *>::error("Unknown source!", ErrorCode::UnknownSourceType);
        }
    }
}
