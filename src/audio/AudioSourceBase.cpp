#include "AudioSourceBase.h"
#include "../core/Config.h"
#include <QDebug>

namespace radar::audio {
    AudioSourceBase::AudioSourceBase(QObject *parent):
        IAudioSource(parent),
        m_isRunning(false)
    {
        auto& config = radar::Config::instance();
        auto configMap = config.audioConfig();
        //若未读取成功则采用默认数值
        m_sampleRate = configMap.value("sampleRate", 44100).toInt();
        m_channels = configMap.value("channels", 1).toUInt();
        m_sampleSize = configMap.value("sampleSize", 16).toUInt();
        m_startTime = 0;
    }

    Result<void> AudioSourceBase::start() {
        if (m_isRunning) {
            return Result<void>::error("Already running!", ErrorCode::InvalidState);
        }
        m_isRunning = true;
        //获取当前时间，精确到毫秒（QDateTime库)
        m_startTime = QDateTime::currentMSecsSinceEpoch();
        return Result<void>::ok();
    }

    Result<void> AudioSourceBase::stop() {
        if (!m_isRunning) {
            return Result<void>::error("Already stopped!", ErrorCode::InvalidState);
        }
        //打印采样时长
        uint64_t duration = getDuration();
        qDebug() << "Audio sampling stopped. Total duration: " << duration << "ms" << endl;
        m_isRunning = false;
        return Result<void>::ok();
    }
}