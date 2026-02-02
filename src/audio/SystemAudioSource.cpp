#include "SystemAudioSource.h"

namespace radar::audio {
    SystemAudioSource::SystemAudioSource(QObject *parent):
        AudioSourceBase(parent),
        m_audioInput(nullptr),
        m_ioDevice(nullptr)
    {}

    Result<void> SystemAudioSource::start() {
        Result<void> baseResult = AudioSourceBase::start();
        if (!baseResult.isOk()) {
            return baseResult;
        }
        if (QAudioDeviceInfo::defaultInputDevice().isNull()) {
            return Result<void>::error("Audio device not found!", ErrorCode::AudioDeviceNotFound);
        }
        m_format.setSampleRate(m_sampleRate);
        m_format.setChannelCount(m_channels);
        m_format.setSampleSize(m_sampleSize);
        //数据格式
        m_format.setCodec("audio/pcm");
        //小端序
        m_format.setByteOrder(QAudioFormat::LittleEndian);
        m_format.setSampleType(QAudioFormat::SignedInt);

        QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
        if (!info.isFormatSupported(m_format)) {
            return Result<void>::error("Unsupported format!", ErrorCode::UnsupportedFormat);
        }

        m_audioInput = new QAudioInput(m_format, this);
        m_ioDevice = m_audioInput->start();
        if (m_ioDevice == nullptr) {
            return Result<void>::error("Failed to start audio input!", ErrorCode::AudioDeviceInitFailed);
        }
        //链接signals和slots
        connect(m_ioDevice, &QIODevice::readyRead, this, &SystemAudioSource::readFrame);

        return Result<void>::ok();
    }

    void SystemAudioSource::readFrame() {
        if (m_ioDevice == nullptr || m_audioInput == nullptr) {
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
        audio_frame.sampleSize = m_sampleSize;
        audio_frame.data = data;
        audio_frame.sourceId = "SystemMicro";
        emit frameReady(audio_frame);
    }

    Result<void> SystemAudioSource::stop() {
        Result<void> baseResult =  AudioSourceBase::stop();
        if (!baseResult.isOk()) {
            return baseResult;
        }
        if (m_audioInput != nullptr) {
            m_audioInput->stop();
            delete m_audioInput;
            m_audioInput = nullptr;
            m_ioDevice = nullptr;
        }
        return Result<void>::ok();
    }
}