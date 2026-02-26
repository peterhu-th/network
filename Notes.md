网络服务：
分层：让代码更清晰
命名不标准
最高层mapper，直接与数据库circle联系
mapper.h/.cpp   提取与数据库有关的逻辑
service.h/.cpp  调用若干个mapper，在controller只需要调用service，封装
controller.h/.cpp   接口：对外暴露地址，接收参数，调用service生成.json

把不参与数据流的放在utils中
在用户变量中新建database的path
未显示指定数据库，使用的是QT自带的，应该使用postgreSQL，可以处理重数据
网络访问测试，成功再上传
QT创建前端页面，要求分页功能和下载

根据要求修改network代码：
1.分层：最高层mapper直接与circle数据库联系，存放与数据库有关的逻辑；service调用若干个mapper，封装；controller接口，对外暴露地址，接收参数，调用service，生成.json
2.把不参与数据流的部分放在utils中作为工具
3.显示指定使用postgreSQL数据库
4.QT创建前端页面，实现分页和下载功能