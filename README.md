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

## 模块需求说明

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

**职责：** 负责连接数据源并产出标准化的音频帧。

**功能需求：**
1.  **多协议支持**: 需支持通过 TCP 或 UDP 接收原始 PCM 数据流（具体协议由配置决定）。
2.  **数据封装**: 将接收到的字节流封装为 `radar::AudioFrame` 结构，包含正确的时间戳和音频参数（采样率、通道等）。
3.  **断线重连**: 在连接断开时应具备自动重连机制。
4.  **错误处理**: 能够识别网络超时或格式错误，并上报状态。

**输入：** 传感器数据流
**输出：** `AudioFrame`
基类：
factory管理两种数据，实现包含两种数据，单独文件管理数据的生命周期，返回类
单独变量管理状态，描述变量，是否连接上，是否正常运行……


---

### Processing 模块（待开发）

**职责：** 清洗数据，识别有效信号。

**功能需求：**
1.  **流水线设计**: 支持多种处理器串行工作（例如：先降噪，再进行静音检测）。
2.  **可扩展性**: 需设计为易于添加新算法的架构。
3.  **信号筛选**: 能够根据配置阈值判断音频帧是否包含有效信号（VAD - Voice Activity Detection）。
4.  **元数据生成**: 计算并附加信号特征（如最大振幅、信噪比等）到 `ProcessedData` 结构中。

**输入：** `AudioFrame`
**输出：** `ProcessedData`

---

### Storage 模块（待开发）

**职责：** 本地文件管理与存储。

**功能需求：**
1.  **格式规范**: 音频数据保存为标准 WAV 格式；元数据保存为同名 JSON 文件。
2.  **目录结构**: 按日期分层存储，例如 `storage/YYYY/MM/DD/audio_<timestamp>.wav`。
3.  **空间管理**: 需监控存储目录大小。当超过配置上限时，依据 FIFO（先进先出）原则自动清理旧文件。
4.  **队列管理**: 提供接口查询“已保存但未上传”的文件列表。

**输入：** `ProcessedData`
**输出：** 文件路径

---

### Network 模块（待开发）

**职责：** 将本地数据同步至云端。

**功能需求：**
1.  **HTTPS 上传**: 将音频文件及其对应的元数据上传至指定 REST API。
2.  **异步处理**: 上传过程不能阻塞主线程。
3.  **可靠性**: 遇到网络波动需支持重试策略。
4.  **状态反馈**: 需明确反馈上传成功、失败、超时等状态。

**输入：** 文件路径 + 元数据
**输出：** 上传状态

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
