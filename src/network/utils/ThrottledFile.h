#ifndef THROTTLED_FILE_H
#define THROTTLED_FILE_H

#include <QFile>
#include <QTimer>
#include <QElapsedTimer>

namespace radar::network {
    // 限速
    class ThrottledFile : public QFile {
        Q_OBJECT
    public:
        explicit ThrottledFile(const QString& name, qint64 speedLimitBytesPerSec, QObject* parent = nullptr)
            : QFile(name, parent), m_speedLimit(speedLimitBytesPerSec) {
            m_timer.start();
        }

    protected:
        // 重载底层 I/O 函数，返回值为读取并放入 data 的字节数，为 0 表示暂时无数据可读但未完成读取
        qint64 readData(char *data, qint64 maxlen) override {
            if (m_speedLimit <= 0) {
                return QFile::readData(data, maxlen);
            }

            // 计算预计花费时间
            qint64 expectedMs = (m_totalBytesRead * 1000) / m_speedLimit;
            qint64 actualMs = m_timer.elapsed();
            if (actualMs < expectedMs) {
                // 延迟触发 readyRead 信号，继续readData
                QTimer::singleShot(expectedMs - actualMs, this, &QIODevice::readyRead);
                return 0;
            }

            // 计算当前时窗允许读取的最大字节数
            qint64 allowedBytes = qMax(static_cast<qint64>(4096), m_speedLimit / 10);
            qint64 toRead = qMin(maxlen, allowedBytes);
            
            qint64 bytesRead = QFile::readData(data, toRead);
            if (bytesRead > 0) {
                m_totalBytesRead += bytesRead;
            }
            return bytesRead;
        }

    private:
        qint64 m_speedLimit = 0;
        qint64 m_totalBytesRead = 0;
        QElapsedTimer m_timer;
    };
}

#endif