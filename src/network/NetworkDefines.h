#ifndef NETWORK_DEFINES_H
#define NETWORK_DEFINES_H

#include <QString>
#include <QDateTime>

namespace radar {
namespace network {

/**
 * @brief 音频记录结构体
 */
struct AudioRecord {
    int64_t id;                 // 雪花算法ID
    QString filePath;           // 文件绝对路径
    QDateTime generationTime;   // 生成时间
    int durationMs;             // 时长(毫秒)
    int64_t fileSize;           // 文件大小(字节)
    QDateTime createdAt;        // 记录创建时间
};

struct DatabaseConfig {
    QString type = "QMYSQL";
    QString host = "127.0.0.1";
    int port = 3306;
    QString dbName = "audio_radar";
    QString username = "root";
    QString password = "";
};

} // namespace network
} // namespace radar

#endif // NETWORK_DEFINES_H
