#include "SystemAudioSource.h"
#include <QMediaDevices>

namespace {
QAudioFormat::SampleFormat toSampleFormat(uint16_t sampleSize)
{
    switch (sampleSize) {
    case 8:
        return QAudioFormat::UInt8;
    case 16:
        return QAudioFormat::Int16;
    case 32:
        return QAudioFormat::Int32;
    default:
        return QAudioFormat::Unknown;
    }
}

int toSampleSize(QAudioFormat::SampleFormat sampleFormat)
{
    switch (sampleFormat) {
    case QAudioFormat::UInt8:
        return 8;
    case QAudioFormat::Int16:
        return 16;
    case QAudioFormat::Int32:
    case QAudioFormat::Float:
        return 32;
    case QAudioFormat::Unknown:
    case QAudioFormat::NSampleFormats:
        return 0;
    }
    return 0;
}
}

namespace radar::audio {
    SystemAudioSource::SystemAudioSource(QObject *parent):
        AudioSourceBase(parent),
        m_audioSource(nullptr),
        m_ioDevice(nullptr)
    {}

    Result<void> SystemAudioSource::start() {
        Result<void> baseResult = AudioSourceBase::start();
        if (!baseResult.isOk()) {
            return baseResult;
        }
        m_audioDevice = QMediaDevices::defaultAudioInput();
        if (m_audioDevice.isNull()) {
            return Result<void>::error("Audio device not found!", ErrorCode::AudioDeviceNotFound);
        }
        m_format = QAudioFormat();
        m_format.setSampleRate(m_sampleRate);
        m_format.setChannelCount(m_channels);
        m_format.setSampleFormat(toSampleFormat(m_sampleSize));
        if (m_format.sampleFormat() == QAudioFormat::Unknown) {
            return Result<void>::error("Unsupported sample size!", ErrorCode::UnsupportedFormat);
        }

        if (!m_audioDevice.isFormatSupported(m_format)) {
            return Result<void>::error("Unsupported format!", ErrorCode::UnsupportedFormat);
        }

        m_audioSource = new QAudioSource(m_audioDevice, m_format, this);
        m_ioDevice = m_audioSource->start();
        if (m_ioDevice == nullptr) {
            return Result<void>::error("Failed to start audio input!", ErrorCode::AudioDeviceInitFailed);
        }
        //链接signals和slots
        connect(m_ioDevice, &QIODevice::readyRead, this, &SystemAudioSource::readFrame);

        return Result<void>::ok();
    }

    void SystemAudioSource::readFrame() {
        if (m_ioDevice == nullptr || m_audioSource == nullptr) {
            return;
        }
        QByteArray data = m_ioDevice->readAll();
        if (data.isEmpty()) {
            return;
        }

        AudioFrame audio_frame;
        audio_frame.timestamp = QDateTime::currentMSecsSinceEpoch();
        audio_frame.sampleRate = m_sampleRate;
        audio_frame.channels = m_channels;
        audio_frame.sampleSize = toSampleSize(m_format.sampleFormat());
        audio_frame.data = data;
        audio_frame.sourceId = "SystemMicro";
        emit frameReady(audio_frame);
    }

    Result<void> SystemAudioSource::stop() {
        Result<void> baseResult =  AudioSourceBase::stop();
        if (!baseResult.isOk()) {
            return baseResult;
        }
        if (m_audioSource != nullptr) {
            m_audioSource->stop();
            delete m_audioSource;
            m_audioSource = nullptr;
            m_ioDevice = nullptr;
        }
        return Result<void>::ok();
    }
}
