#ifndef NETWORK_DEFINES_H
#define NETWORK_DEFINES_H

#include <QDateTime>
#include <cstdint>

namespace radar::network {
    struct AudioRecord {
        int64_t id = 0;             // 雪花算法
        QString filePath;           // 绝对路径
        QDateTime generationTime;   // 文件时间
        int duration = 0;           // 音频长度
        int64_t fileSize = 0;       // 字节数
        QDateTime createdAt;        // 记录时间
    };

    struct DatabaseConfig {
        QString type = "QPSQL";         // 驱动
        QString host = "127.0.0.1";     // 环回地址，指向本机
        int port = 5432;                // PostgreSQL 默认端口号
        QString dbName = "audio";
        QString username = "postgres";
        QString password = "6";
    };
}

#endif