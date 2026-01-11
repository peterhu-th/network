#ifndef AUDIOSOURCEBASE_H
#define AUDIOSOURCEBASE_H

#include "IAudioSource.h"
#include <QMutex>

namespace radar {

class AudioSourceBase : public IAudioSource {
    Q_OBJECT

public:
    explicit AudioSourceBase(QObject* parent = nullptr);
    ~AudioSourceBase() override = default;

    Result<void> start() override final;
    Result<void> stop() override final;

    bool isConnected() const override;
    bool isRunning() const override;

protected:
    virtual Result<void> doStart() = 0;
    virtual Result<void> doStop() = 0;

    void setConnected(bool connected);
    void emitFrame(const AudioFrame& frame);
    void emitError(const QString& error, ErrorCode code);

private:
    mutable QMutex m_mutex;
    bool m_connected = false;
    bool m_running = false;
};

}

#endif
