#include "Logger.h"
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <iostream>

namespace radar {

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::setLevel(LogLevel level) {
    m_level = level;
}

void Logger::setLogFile(const QString& path) {
    m_logFilePath = path;
}

void Logger::debug(const QString& module, const QString& message) {
    log(LogLevel::Debug, module, message);
}

void Logger::info(const QString& module, const QString& message) {
    log(LogLevel::Info, module, message);
}

void Logger::warning(const QString& module, const QString& message) {
    log(LogLevel::Warning, module, message);
}

void Logger::error(const QString& module, const QString& message) {
    log(LogLevel::Error, module, message);
}

void Logger::log(LogLevel level, const QString& module, const QString& message) {
    if (level < m_level) {
        return;
    }

    static const char* levelStr[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    auto timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    auto line = QString("[%1] [%2] [%3] %4")
        .arg(timestamp)
        .arg(levelStr[static_cast<int>(level)])
        .arg(module)
        .arg(message);

    std::cout << line.toStdString() << std::endl;

    if (!m_logFilePath.isEmpty()) {
        QFile file(m_logFilePath);
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << line << "\n";
        }
    }
}

}