#ifndef ZIP_UTILS_H
#define ZIP_UTILS_H

#include <QString>
#include <QFileInfo>
#include <QDebug>
#include "../../../third-party/miniz/miniz.h"
#include "../../core/Types.h"

namespace radar::network {
    class ZipUtils {
    public:
        /**
         * @brief 将多个本地文件打包成一个 ZIP 压缩包
         * @param sourceFiles 源文件的绝对路径列表
         * @param destZipPath 目标 ZIP 文件的绝对路径
         * @return Result<void> 成功或包含错误信息的 Result
         */
        static Result<void> createZip(const QList<QString>& sourceFiles, const QString& destZipPath) {
            mz_zip_archive zip_archive;
            mz_zip_zero_struct(&zip_archive);
            // 初始化 ZIP 写入器
            if (!mz_zip_writer_init_file(&zip_archive, destZipPath.toLocal8Bit().constData(), 0)) {
                return Result<void>::error("Failed to initialize zip archive", ErrorCode::ToolsError);
            }
            // 遍历添加文件
            for (const QString& filePath : sourceFiles) {
                QFileInfo info(filePath);
                if (!info.exists() || !info.isFile()) {
                    qWarning() << "ZipUtils: Source file does not exist or is not a file, skipping:" << filePath;
                    continue;
                }
                QByteArray archiveName = info.fileName().toUtf8();
                QByteArray srcFilename = filePath.toLocal8Bit();
                if (!mz_zip_writer_add_file(&zip_archive,
                                            archiveName.constData(),
                                            srcFilename.constData(),
                                            nullptr, 0, MZ_DEFAULT_COMPRESSION)) {
                    mz_zip_writer_end(&zip_archive);
                    return Result<void>::error("Failed to add file to zip: " + filePath, ErrorCode::ToolsError);
                }
            }
            // 写入中央目录结束归档
            if (!mz_zip_writer_finalize_archive(&zip_archive)) {
                mz_zip_writer_end(&zip_archive);
                return Result<void>::error("Failed to finalize zip archive", ErrorCode::ToolsError);
            }
            // 关闭文件并释放内部内存
            if (!mz_zip_writer_end(&zip_archive)) {
                return Result<void>::error("Failed to cleanly end zip archive writing", ErrorCode::ToolsError);
            }
            return Result<void>::ok();
        }
    };
}

#endif