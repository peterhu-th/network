# Network 模块说明

## 功能
``` aiignore
    1. 扫描本地文件，每 10 分钟更新一次数据库(FileIndexer)
    2. 按照时间顺序列出 /data 中的数据，提供登录、查询和异步下载 api
    3. 使用 taglib 工具解析文件时长
```

## 配置要求

在 msys2 minGW x64 终端中安装 taglib 包

```bash
pacman -S mingw-w64-x86_64-taglib mingw-w64-x86_64-pkgconf
```