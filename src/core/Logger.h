#ifndef LOGGER_H
#define LOGGER_H

#include <QString>

namespace radar {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger {
public:
    static Logger& instance();

    void setLevel(LogLevel level);
    void setLogFile(const QString& path);

    void debug(const QString& module, const QString& message);
    void info(const QString& module, const QString& message);
    void warning(const QString& module, const QString& message);
    void error(const QString& module, const QString& message);

private:
    Logger() = default;
    void log(LogLevel level, const QString& module, const QString& message);

    LogLevel m_level = LogLevel::Info;
    QString m_logFilePath;
};

}

#define LOG_DEBUG(module, msg) radar::Logger::instance().debug(module, msg)
#define LOG_INFO(module, msg) radar::Logger::instance().info(module, msg)
#define LOG_WARNING(module, msg) radar::Logger::instance().warning(module, msg)
#define LOG_ERROR(module, msg) radar::Logger::instance().error(module, msg)

#endif