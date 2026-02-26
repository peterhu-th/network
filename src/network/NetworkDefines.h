#ifndef NETWORK_DEFINES_H
#define NETWORK_DEFINES_H

#include <QDateTime>

namespace radar::network {
    struct AudioRecord {
        int64_t id = 0;             // 雪花算法ID
        QString filePath;           // 文件绝对路径
        QDateTime generationTime;   // 生成时间
        int durationMs = 0;         // 音频长度
        int64_t fileSize = 0;       // 文件大小(字节)
        QDateTime createdAt;        // 记录创建时间
    };

    struct DatabaseConfig {
        QString type = "QPSQL";
        QString host = "127.0.0.1";     // 指向本机
        int port = 5432;                // PostgreSQL 默认端口
        QString dbName = "circle";
        QString username = "postgres";
        QString password = "";
    };
}

#endif // NETWORK_DEFINES_H
