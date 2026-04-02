# Network 模块说明

## 功能
``` aiignore
    1. 扫描本地文件，每 10 分钟更新一次数据库(FileIndexer)
    2. 按照时间顺序列出 /data 中的数据，提供查询和异步下载 api
    3. 使用 ffprobe 工具解析文件时长
```

## 配置要求
```aiignore
    从 https://www.gyan.dev/ffmpeg/builds/ 下载 ffmpeg-8.0.1-essentials_build.zip
    解压后将 bin/ffprobe.exe 移动至 tools/
```