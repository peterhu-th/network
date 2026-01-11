#ifndef IAUDIOSOURCE_H
#define IAUDIOSOURCE_H

#include <QObject>
#include "core/Types.h"

namespace radar {

class IAudioSource : public QObject {
    Q_OBJECT

public:
    virtual ~IAudioSource() = default;

    virtual Result<void> start() = 0;
    virtual Result<void> stop() = 0;
    virtual Result<AudioFrame> readFrame() = 0;

    virtual bool isConnected() const = 0;
    virtual bool isRunning() const = 0;

signals:
    void frameReady(const AudioFrame& frame);
    void errorOccurred(const QString& error, ErrorCode code);
    void stateChanged(bool connected, bool running);
};

}

#endif
