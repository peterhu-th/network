# Audio-Radar-Client 开发文档

## 概述

雷达音频信号采集客户端。从系统音频线接收 PCM 数据，经 Audio/Processing 处理后写入本地存储，并由本地网络服务提供下载能力（不上传服务器）。

## 项目结构

```
src/
├── main.cpp           # 程序入口（加载 config，创建 Audio/Processing）
├── core/              # 基础框架（Config/Logger/Types）
├── audio/             # 已实现：系统音频采集 + AudioSourceFactory
├── processing/        # 已实现：Processor/Pipeline/Service/Factory
├── storage/           # 本地存储（待开发）
└── network/           # 本地网络服务（待开发）
config/
tests/
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
系统音频线 → Audio → Processing → Storage → Network
                │          │           │          │
            AudioFrame  ProcessedData  FilePath  Download API
```

---

## 模块需求说明

### Core 模块（已实现）

基础类型与配置/日志支持。

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

### Audio 模块（已实现）

**职责：** 连接系统音频输入并产出标准化音频帧。

**现有实现：**
1. **IAudioSource/AudioSourceBase**：音频源接口与基础类（状态管理/采样参数）
2. **SystemAudioSource**：基于 Qt `QAudioInput` 的系统音频采集
3. **AudioSourceFactory**：根据 `config.audio.type` 选择实现（当前仅 `system`）

**输入：** 系统音频线 PCM
**输出：** `AudioFrame`


---

### Processing 模块（已实现）

**职责：** 对 `AudioFrame` 做流水线处理并生成 `ProcessedData`。

**现有实现：**
1. **Processor 接口**：`process(const AudioFrame&) -> Result<ProcessedData>`
2. **处理器**：`DenoiseProcessor` / `VADProcessor` / `FeatureExtractor`
3. **PipelineManager**：串行执行处理器链
4. **AudioProcessingService**：封装 pipeline，对外提供 `processAudioFrame`
5. **ProcessorFactory**：创建默认流水线（Denoise → VAD → Feature）

**输入：** `AudioFrame`
**输出：** `ProcessedData`

---

### Storage 模块（待开发）

**职责：** 本地文件管理与存储（目前代码未实现）。

**功能需求：**
1.  **格式规范**: 音频数据保存为标准 WAV 格式；元数据保存为同名 JSON 文件。
2.  **目录结构**: 按日期分层存储，例如 `storage/YYYY/MM/DD/audio_<timestamp>.wav`。
3.  **空间管理**: 需监控存储目录大小。当超过配置上限时，依据 FIFO（先进先出）原则自动清理旧文件。
4.  **队列管理**: 提供接口查询“已保存待提供下载”的文件列表。

**输入：** `ProcessedData`
**输出：** 文件路径

---

### Network 模块（已实现）

**职责：** 在本地提供网络服务，支持下载本地文件与元数据（不上传服务器）。

**功能需求：**
1.  **本地下载**: 提供 HTTP API 或本地协议，支持下载 WAV/JSON
2.  **异步处理**: 下载/读取不阻塞主线程
3.  **可扩展性**: 预留鉴权、限速、断点续传等策略

**输入：** 文件路径 + 元数据
**输出：** 下载响应/文件流

---

## 配置文件

`config/config.json`:

```json
{
    "audio": {
        "type": "system",
        "sampleRate": 44100,
        "channels": 1,
        "bitsPerSample": 16,
        "device": "default"
    },
    "storage": {},
    "network": {}
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
