#include "AudioSourceBase.h"
#include <QMutexLocker>

namespace radar {

AudioSourceBase::AudioSourceBase(QObject* parent)
    : IAudioSource(parent)
{
}

Result<void> AudioSourceBase::start()
{
    {
        QMutexLocker locker(&m_mutex);
        if (m_running) {
            return Result<void>::error("Audio source already running", ErrorCode::Success);
        }
    }

    auto result = doStart();
    if (result.isOk()) {
        QMutexLocker locker(&m_mutex);
        m_running = true;
        emit stateChanged(m_connected, m_running);
    }
    return result;
}

Result<void> AudioSourceBase::stop()
{
    {
        QMutexLocker locker(&m_mutex);
        if (!m_running) {
            return Result<void>::ok();
        }
    }

    auto result = doStop();
    {
        QMutexLocker locker(&m_mutex);
        m_running = false;
        emit stateChanged(m_connected, m_running);
    }
    return result;
}

bool AudioSourceBase::isConnected() const
{
    QMutexLocker locker(&m_mutex);
    return m_connected;
}

bool AudioSourceBase::isRunning() const
{
    QMutexLocker locker(&m_mutex);
    return m_running;
}

void AudioSourceBase::setConnected(bool connected)
{
    QMutexLocker locker(&m_mutex);
    if (m_connected != connected) {
        m_connected = connected;
        emit stateChanged(m_connected, m_running);
    }
}

void AudioSourceBase::emitFrame(const AudioFrame& frame)
{
    emit frameReady(frame);
}

void AudioSourceBase::emitError(const QString& error, ErrorCode code)
{
    emit errorOccurred(error, code);
}

}
