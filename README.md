# Audio-Radar-Client 开发文档

## 概述

雷达音频信号采集客户端。从传感器接收音频数据，处理后上传至后端和大模型。

## 项目结构

```
src/
├── main.cpp           # 程序入口
├── core/              # 基础框架（已实现）
├── audio/             # 音频采集（待开发）
├── processing/        # 信号处理（待开发）
├── storage/           # 本地存储（待开发）
└── network/           # 网络通信（待开发）
```

## 构建

```bash
mkdir build && cd build
cmake ..
make
```

环境要求：Qt5, CMake 3.16+, C++17+

## 数据流

```
传感器 → Audio → Processing → Storage → Network → Go API
            │          │           │          │
        AudioFrame  ProcessedData  FilePath  HTTPS
```

---

## 模块说明

### Core 模块（已实现）

所有模块的基础设施。

**公共数据结构（Types.h）：**

```cpp
namespace radar {

struct AudioFrame {
    int64_t timestamp;       // 时间戳（微秒）
    uint32_t sampleRate;     // 采样率
    uint16_t channels;       // 声道数
    uint16_t bitsPerSample;  // 位深
    QByteArray data;         // PCM原始数据
    QString sourceId;        // 数据源标识
};

struct ProcessedData {
    AudioFrame originalFrame;
    bool isValid;            // 是否有效信号
    double signalStrength;   // 信号强度
    QVariantMap features;    // 特征数据
};

template<typename T>
class Result {
    static Result<T> ok(const T& value);
    static Result<T> error(const QString& message, ErrorCode code);
    bool isOk() const;
    T value() const;
};

}
```

---

### Audio 模块（待开发）

**职责：** 从传感器接收原始音频数据（TCP/UDP）

**输入：** 传感器数据流
**输出：** `AudioFrame`

**需实现的接口：**

```cpp
class IAudioSource : public QObject {
    Q_OBJECT
public:
    virtual bool open(const SourceConfig& config) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;

signals:
    void frameReady(const AudioFrame& frame);
    void errorOccurred(const QString& error);
};
```

**交付物：**
- IAudioSource.h - 接口定义
- AudioReceiver.h/cpp - TCP/UDP实现
- 单元测试

---

### Processing 模块（待开发）

**职责：** 对音频信号进行筛选、过滤

**输入：** `AudioFrame`
**输出：** `ProcessedData`

**需实现的接口：**

```cpp
class IProcessor {
public:
    virtual QString name() const = 0;
    virtual bool process(const AudioFrame& input, ProcessedData& output) = 0;
};

class ProcessingPipeline : public QObject {
    Q_OBJECT
public:
    void addProcessor(std::shared_ptr<IProcessor> processor);
    void process(const AudioFrame& frame);

signals:
    void completed(const ProcessedData& data);
    void rejected(const AudioFrame& frame, const QString& reason);
};
```

**交付物：**
- IProcessor.h - 接口定义
- ProcessingPipeline.h/cpp - 流水线管理
- SignalFilter.h/cpp - 信号过滤
- 单元测试

---

### Storage 模块（待开发）

**职责：** 将处理后的数据存储到本地

**输入：** `ProcessedData`
**输出：** 文件路径

**需实现的接口：**

```cpp
class IStorage {
public:
    virtual Result<QString> save(const ProcessedData& data) = 0;
    virtual Result<ProcessedData> load(const QString& id) = 0;
    virtual QStringList listPending() = 0;  // 待上传文件列表
};
```

**存储格式：**
```
storage/
└── YYYY/MM/DD/
    ├── audio_<timestamp>.wav
    └── audio_<timestamp>.json  # 元数据
```

**交付物：**
- IStorage.h - 接口定义
- StorageManager.h/cpp - 存储管理
- AudioFileWriter.h/cpp - WAV文件写入
- 单元测试

---

### Network 模块（待开发）

**职责：** 与 Go Web API 通信，上传数据

**输入：** 文件路径 + 元数据
**输出：** 上传状态

**需实现的接口：**

```cpp
class ApiService : public QObject {
    Q_OBJECT
public:
    void setBaseUrl(const QString& url);
    QFuture<Result<QString>> uploadAudio(const QString& filePath,
                                          const QVariantMap& metadata);
signals:
    void uploadCompleted(const QString& fileId, const QString& serverId);
    void uploadFailed(const QString& fileId, const QString& error);
};
```

**交付物：**
- HttpClient.h/cpp - HTTPS客户端
- ApiService.h/cpp - API封装
- 重试机制
- 单元测试

---

## 配置文件

`config/config.json`:

```json
{
    "audio": {
        "type": "tcp",
        "host": "127.0.0.1",
        "port": 8080
    },
    "storage": {
        "path": "./data",
        "maxSize": 10737418240
    },
    "network": {
        "baseUrl": "https://api.example.com",
        "timeout": 30000
    }
}
```

---

## 开发流程

1. Fork 仓库
2. 创建功能分支：`git checkout -b feature/模块名`
3. 按接口定义实现模块
4. 在根 `CMakeLists.txt` 中取消对应模块的注释
5. 编写单元测试
6. 提交 PR 进行代码评审
