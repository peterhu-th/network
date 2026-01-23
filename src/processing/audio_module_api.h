#ifndef AUDIO_MODULE_API_H
#define AUDIO_MODULE_API_H

#include "radar_types.h"

namespace audio_module {
    // Audio模块核心接口：读取一帧音频数据
    radar::AudioFrame readAudioFrame();
}

#endif // AUDIO_MODULE_API_H