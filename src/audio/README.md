# Audio-Radar-Client 开发文档 （Audio部分）

## 概述

audio模块可从PCM/TCP/UDP源接收原始音频数据 ，采用Factory模式，提供统一的接口IAudioSource

## 核心架构

```
classDiagram
    class IAudioSource {
        <<Interface>>
        +start() Result
        +stop() Result
        +readFrame() Result~AudioFrame~
        +getSampleRate() uint32
        -- Signals --
        +frameReady(AudioFrame)
        +errorOccurred(QString, ErrorCode)
    }

    class AudioSourceBase {
        <<Abstract>>
        -m_frameBuffer: QList~AudioFrame~
        -m_reconnectTimer: QTimer
        #processData(QByteArray)
        #emitFrame(AudioFrame)
    }

    class TcpAudioSource {
        -m_socket: QTcpSocket
    }

    class UdpAudioSource {
        -m_socket: QUdpSocket
    }
    
    class SystemAudioSource {
        -m_audioInput: QAudioSource
        -m_ioDevice: QIODevice
    }

    class AudioSourceFactory {
        +create() IAudioSource*
    }

    IAudioSource <|-- AudioSourceBase
    AudioSourceBase <|-- TcpAudioSource
    AudioSourceBase <|-- UdpAudioSource
    AudioSourceBase <|-- SystemAudioSource
    AudioSourceFactory ..> IAudioSource : Creates
```

## 文件索引

|文件名| 类型  |职责|
|---|-----|---|
|IAudioSource.h| 接口  |定义音频源必须实现的公共接口（启动、停止、读取、状态查询）及标准信号|
|AudioSourceBase.h/cpp| 基类  |实现通用逻辑：<br/>1.缓存管理(FIFO 队列)<br/>2.线程安全(Mutex锁)<br/>3.断线重连机制<br/>4.数据标准化|
|TcpAudioSource.h/cpp| 实现类 |基于QTcpSocket。作为客户端连接远程雷达/服务器，接收连续音频流|
|UdpAudioSource.h/cpp|实现类|基于QUdpSocket。监听特定端口，接收无连接的音频数据包|
|SystemAudioSource.h/cpp|实现类|基于Qt Multimedia(QAudioSource)。直接从计算机的Line-In接口采集PCM数据|
|AudioSourceFactory.h/cpp|工厂|根据config.json中的type字段 ("tcp", "udp", "system")动态创建对应的采集对象|

## 配置说明

"audio": {

"type": "system",          // 可选"system", "tcp", "udp"

"sampleRate": 44100,       // 采样率

"channels": 1,             // 通道数 (1=单声道, 2=立体声)

"bitsPerSample": 16,       // 位深 (8, 16, 32)

}

