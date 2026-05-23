# Network 模块说明

## 功能
``` aiignore
    1. 扫描本地文件，每 10 分钟更新一次数据库(FileIndexer)
    2. 注册/登录：基于 SMTP 的邮箱验证注册，密码登录
    3. 查询：分页显示，支持根据文件类型和记录时间筛选
    4. 下载：提供单个文件下载和多文件 zip 下载
    5. 管理：管理员可管理其他账号权限、删除文件
```

## 配置要求

### 在 msys2 minGW x64 终端中安装 taglib 包

```bash
pacman -S mingw-w64-x86_64-taglib mingw-w64-x86_64-pkgconf
```

### 在 third-party 中导入 jwt-cpp 和 miniz 3.1.0

```
third-party/
├── jwt-cpp/include/
    ├── jwt-cpp
      ├── base.h
      ├── jwt.h
    ├── picojson/picojson.h
├── miniz
  ├──miniz.c
  ├──miniz.h
```