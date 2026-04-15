#ifndef NETWORK_DTO_H
#define NETWORK_DTO_H

#include <QJsonObject>
#include <QJsonArray>
#include <QMetaProperty>

namespace radar::network {
    // 单条数据
    struct AudioRecordDTO {
        Q_GADGET    // 允许拷贝，适用于 DTO 和配置结构体
        
        // 注册需要反射的属性
        // 语法：Q_PROPERTY(暴露类型 暴露属性名 READ 成员函数)
        Q_PROPERTY(QString id READ getId)
        Q_PROPERTY(QString fileSize READ getFileSize)
        Q_PROPERTY(QString generationTime READ getGenerationTime)
        // 绑定成员变量，可不通过成员函数直接访问或修改，适用于不需要类型转换和数据校验的变量
        // 语法：Q_PROPERTY(类型 变量名 MEMBER 成员变量名)
        Q_PROPERTY(int duration MEMBER duration)

    public:
        qint64 id = 0;
        int duration = 0;
        qint64 fileSize = 0;
        QDateTime generationTime;

        // JS 不支持 64 位整数，转为 QString 保留精度
        [[nodiscard]] QString getId() const { return QString::number(id); }
        [[nodiscard]] QString getFileSize() const { return QString::number(fileSize); }
        [[nodiscard]] QString getGenerationTime() const { return generationTime.toString(Qt::ISODate); }

        // 遍历结构体中所有已注册的属性并打包生成 JSON 对象
        [[nodiscard]] QJsonObject toJson() const {
            QJsonObject obj;
            // staticMetadataObject 是 Q_GADGET 自动生成的静态成员，包含类所有属性和方法
            const QMetaObject* metaObj = &staticMetaObject;
            for (int i = 0; i < metaObj->propertyCount(); ++i) {
                // QMetaProperty 类包括属性名、类型、读写性和读取/修改方法
                QMetaProperty prop = metaObj->property(i);
                obj.insert(QString::fromLatin1(prop.name()), QJsonValue::fromVariant(prop.readOnGadget(this)));
            }
            return obj;
        }
    };

    // 包装列表的 DTO
    template <typename T>
    struct PageDTO {
        int total = 0;
        std::vector<T> list;

        [[nodiscard]] QJsonObject toJson() const {
            QJsonObject obj;
            obj["total"] = total;
            QJsonArray arr;
            for (const auto& item : list) {
                arr.append(item.toJson());
            }
            obj["list"] = arr;
            return obj;
        }
    };

    struct AudioRecord {
        int64_t id = 0;             // 雪花算法
        QString filePath;           // 绝对路径
        QDateTime generationTime;   // 文件时间
        int duration = 0;           // 音频长度
        int64_t fileSize = 0;       // 字节数
        QDateTime createdAt;        // 记录时间
    };

    struct DatabaseConfig {
        QString type;         // 驱动
        QString host;         // 指向本机
        int port = 0;         // PostgreSQL 默认端口号
        QString dbName;
        QString username;
        QString storagePath;
        QString ffprobePath;  // 工具路径
        QString passWord;
    };

    struct NetworkConfig {
        QString bindAddress;
        int port = 0;
        QString serverSecret;
        QString globalConnectionName;
    };

    // Controller 与 Service 之间的 DTO
    struct FileDownloadContext {
        QIODevice* stream = nullptr;
        QString fileName;
        QString contentType;
        qint64 fileSize = 0;
        qint64 startPos = 0;
        qint64 endPos = 0;
        bool isPartial = false;
    };

}

#endif