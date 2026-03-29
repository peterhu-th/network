#include "Config.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace radar {

Config& Config::instance() {
    static Config instance;
    return instance;
}

bool Config::load(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        return false;
    }

    m_config = doc.object().toVariantMap();
    return true;
}

QString Config::getString(const QString& key, const QString& defaultValue) const {
    return m_config.value(key, defaultValue).toString();
}

int Config::getInt(const QString& key, int defaultValue) const {
    return m_config.value(key, defaultValue).toInt();
}

bool Config::getBool(const QString& key, bool defaultValue) const {
    return m_config.value(key, defaultValue).toBool();
}

QVariantMap Config::audioConfig() const {
    return m_config.value("audio").toMap();
}

QVariantMap Config::storageConfig() const {
    return m_config.value("storage").toMap();
}

QVariantMap Config::networkConfig() const {
    return m_config.value("network").toMap();
}

QVariantMap Config::databaseConfig() const {
    return m_config.value("database").toMap();
}

QString Config::authToken() const {
    return networkConfig().value("authToken", "").toString();
}

}
