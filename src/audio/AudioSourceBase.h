#ifndef AUDIO_SOURCE_BASE_H
#define AUDIO_SOURCE_BASE_H

#include "IAudioSource.h"
#include <QDateTime>

namespace radar::audio {
    class AudioSourceBase: public IAudioSource {
        Q_OBJECT

    public:
        explicit AudioSourceBase(QObject *parent = nullptr);
        ~AudioSourceBase() override = default;
        //开始采样
        Result<void> start() override;
        //结束采样
        Result<void> stop() override;
        //getter
        int getSampleRate() override { return m_sampleRate;}
        uint16_t getChannels() override { return m_channels;}
        uint16_t getSampleSize() override { return m_sampleSize;}
        uint64_t getDuration() override { return QDateTime::currentMSecsSinceEpoch() - m_startTime;}

    protected:
        bool m_isRunning;
        //为了SystemAudioSource::start()中赋值类型一致，m_sampleRate改为int类型
        int m_sampleRate;
        uint16_t m_channels;
        uint16_t m_sampleSize;
        //开始时间戳，用于计算duration
        int64_t m_startTime;
    };
}
#endif