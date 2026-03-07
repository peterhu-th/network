一、 核心问题剖析 (Analysis)
1. 内存/生命周期问题：Lambda 捕获与异步 I/O 的“死亡交叉”
   在现有的 handleDownload 中，如果采用 C++11 的 Lambda 表达式结合 QTcpSocket::bytesWritten 信号来做分块发送，存在极高的野指针风险。

原因：TCP Socket 的断开是纯异步的，客户端随时可能掐断连接。如果此时 Lambda 还在排队执行，或者刚刚进入闭包准备读取 QFile*，但 Socket 和附加在它身上的对象已经被清理，就会发生经典的“Use-After-Free”段错误（Segmentation Fault）。

对策评估：你提出的引入 DownloadSession（下载会话类） 是极其标准且优雅的 Qt 解决范式。通过继承 QObject，将 QTcpSocket 和 QFile 作为它的内部成员或子对象（Parent-Child 树），当下载完成或 Socket 断开时，调用 this->deleteLater()，让 Qt 的事件循环去安全地回收整个会话的内存。这彻底消灭了局部变量和智能指针在 Lambda 中的捕获灾难。

2. 鉴权裸奔：硬编码 Token 漏洞
   原因：将密码或 Token ("user") 写死在 C++ 源码中，不仅意味着任何人拿到编译后的执行文件都能通过简单的 strings 命令逆向出秘钥，还意味着部署在不同现场的雷达设备共享同一个后门密码，毫无隔离性可言。

对策评估：必须剥离硬编码，将其移入系统的配置中心（如 config.json），在服务启动时动态加载。

3. HTTP 解析脆弱与 CORS 跨域规范限制
   原因：

按照单空格分割 HTTP 请求行（如 GET /api HTTP/1.1）过于理想化，遇到非标客户端或恶意的畸形报文会导致数组越界或解析崩溃。

在浏览器 W3C 规范中，如果前端发起的请求带有身份凭证（如 Cookie 或 Authorization 头），后端的跨域响应头 Access-Control-Allow-Origin 绝对不允许设置为通配符 *，必须是具体的域名（如 http://192.168.1.100:5500）。这会导致前端出现跨域阻断错误。

二、 分步修改计划 (Modification Plan)
为了稳妥地修复这些底层漏洞并提升架构健壮性，建议按照以下三个阶段进行重构：

阶段一：动态化鉴权机制 (解决安全裸奔)
修改配置文件：在 config/config.json 的 network 节点中增加一个 auth_token 字段（例如 "auth_token": "radar_secure_2024"）。

修改 Config 类：在 src/core/Config.h/cpp 中增加对 auth_token 的读取和全局存储支持。

重构鉴权验证：修改 AudioRecordController::checkAuthorization，把原来 if (token != "user") 的硬编码，替换为与 Config 类中读取到的真实 Token 进行比对。前端 api.js 中的常量也需同步修改。

阶段二：重构 HTTP 解析与严谨化跨域头 (解决协议脆弱性)
增强请求行解析：修改 HttpServer::parseRequest，使用正则表达式（如 QRegularExpression("\\s+")）或安全的字符串剔除逻辑来应对多余空格。增加对数组长度的边界检查，如果解析出的片段少于 3 个（Method, Path, Version），直接返回 400 Bad Request。

动态跨域头 (CORS)：

在处理 OPTIONS 预检请求或普通响应时，不再直接返回 Access-Control-Allow-Origin: *。

逻辑变更为：读取请求头中的 Origin 字段，如果存在，则直接将这个具体的 Origin 回写到响应头的 Access-Control-Allow-Origin 中，并加上 Access-Control-Allow-Credentials: true。

阶段三：引入 DownloadSession 面向对象解耦 (解决内存与野指针风险)
创建新类：在 src/network/controller/ 目录下新建 DownloadSession.h 和 DownloadSession.cpp。

状态封装：该类继承自 QObject，成员变量包含 QTcpSocket* m_socket、QFile* m_file、qint64 m_totalBytes、qint64 m_bytesSent 等。

接管信号流：

在其内部的 start() 方法中，建立与 Socket bytesWritten 信号以及 disconnected 信号的连接。

将之前散落在 AudioRecordController 中的断点续传（Range 解析）、文件打开、响应头拼接、分块发送 (read + write) 逻辑全部搬移进这个类。

安全销毁机制：

当文件发送完毕，或者捕获到 Socket 的 disconnected / errorOccurred 信号时，主动调用 m_socket->disconnectFromHost()，随后执行 this->deleteLater() 实现内存自我安全回收。

瘦身 Controller：在 AudioRecordController::handleDownload 中，前期只做参数校验和鉴权。校验通过后，直接 new DownloadSession(socket, filePath, rangeStart) 并调用 start()，Controller 即可立刻 return，将后续繁重的异步发送生命周期全权交由 Session 对象自我管理。