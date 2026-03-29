1. 把controller的逻辑放在service中，mapper对外提供与数据库交互的接口，主要逻辑集中在service
2. 不用字符串处理param和header
3. 定义network错误类型
4. 封装transfer data object作为前后端数据交流
5. 使用vue框架
6. 去掉 URL token，只保留 Authorization 头
   把硬编码 token 和数据库密码移出代码。
   默认只监听 127.0.0.1，不要监听全部网卡。
   不再向前端返回 filePath 绝对路径。
   统一 HTTP 状态码和 JSON 响应格式