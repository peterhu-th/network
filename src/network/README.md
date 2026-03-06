1. Network 模块的功能与接口简述
   network 模块的主要职责是提供一个本地 HTTP 服务，支持异步非阻塞的文件元数据查询与大文件流式下载功能。目前对外提供两个核心 API 接口：

文件列表查询接口 (GET /api/files)
功能: 支持前端获取已保存的音频记录列表。
参数: 支持 limit（每页条目数）、offset（偏移量）进行分页；支持 startTime、endTime 进行时间段过滤。

文件下载接口 (GET /api/download)
功能: 支持前端下载音频文件或其配套的 JSON 元数据。
特性: 实现了断点续传（处理 Range 请求头）和基于 Qt 事件循环的异步分块限速下载。
参数: id（文件唯一标识）、type（可选，设为 json 下载元数据，否则默认下载 wav）。

2. 可能改变的配置/参数位置
   未来根据联调需求，你可能需要修改以下参数的默认值，它们分布在不同的文件中：

全局配置 (端口、存储路径、数据库参数):
位于 config/config.json 中。包含了 network.port (默认 8080) 和 storage.path。
前端分页条目数:
位于 src/ui/js/main.js。代码中的 const pageSize = 20; 规定了前端每次请求的数据量。
文件自动扫描间隔:
位于 src/network/utils/FileIndexer.h。start 函数的默认参数 intervalMs 决定了每隔多久扫描一次本地目录。
测试环境固定路径:
位于 tests/test_network.cpp。集成测试中硬编码了 QString testDataPath = "./data"; 用于生成和读取测试数据。

3. 目前依赖“模拟 (Mock)”机制的代码
   由于 storage 尚未完工，且为了避免物理数据库环境阻断联调，当前有一些“桩代码”未来需要替换或对接：

内存数据库 Mock:
src/network/mapper/AudioRecordMapper.cpp 完全采用了基于 std::map 和 std::vector 的内存结构来模拟增删改查。未来需要切换为真正的 QSqlDatabase 和 SQL 语句。
测试数据模拟生成:
在 tests/test_network.cpp 中，手动通过代码向本地写入了伪造的 dummy.wav 和 dummy.json 文件以触发扫描逻辑。对接后，这部分数据将直接来源于 processing 到 storage 的真实写入。

4. 注释中标记未实现 / TODO 的代码
   当前代码中有明确留待后续补充的部分，你在联调时需要特别关注：
读取音频文件时长:
在 src/network/utils/FileIndexer.cpp 中，元数据解析函数里写有 record.durationMs = 0; // 需要读取 wav 头获取时长，这里暂时略过 或 record.duration = 0; //TODO: 读取 wav 文件获取时长。这需要 storage 模块或通用工具类提供一个读取 WAV 头部时长信息的解析工具。

5. Network 对 Storage 模块的功能需求明确
   依据当前的架构设计，network 模块能够正常工作的前提是 storage 模块严格遵守以下约定：
双文件同名规范: 存储模块必须将音频保存为标准 WAV 格式，并将对应的元数据特征保存为同名且同目录的 JSON 文件。这是前端能够实现“WAV+JSON”双文件下载功能的前提。
分层目录结构: 存储路径不应所有文件堆叠在一个文件夹下，应按日期组织，例如 storage/YYYY/MM/DD/audio_<timestamp>.wav。FileIndexer 的扫描逻辑已经支持了遍历子目录（QDirIterator::Subdirectories），这方面无缝兼容。
容量自动清理 (FIFO): 当存储达到上限时，storage 模块会自动清理旧文件。这对 network 的影响是：底层的物理文件可能随时丢失。目前网络层在下载接口中加入了文件存在性检查（返回 HTTP 404），因此即便 storage 物理删除了文件，前端也能有合理的错误提示。