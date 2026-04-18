#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QtGlobal>
#include "Config.h"

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
    QJsonObject rootObj = doc.object();
    m_config = doc.object().toVariantMap();

    QJsonObject dbObj = rootObj["database"].toObject();
    m_dbConfig.type = dbObj["type"].toString("QPSQL");
    m_dbConfig.host = dbObj["host"].toString("127.0.0.1");
    m_dbConfig.port = dbObj["port"].toInt(5432);
    m_dbConfig.dbName = dbObj["dbName"].toString("audio");
    m_dbConfig.username = dbObj["username"].toString();
    QString envPassword = qEnvironmentVariable("AUDIO_DB_PASSWORD");
    m_dbConfig.passWord = envPassword.isEmpty() ? dbObj["passWord"].toString() : envPassword;
    QString envStoragePath = qEnvironmentVariable("AUDIO_STORAGE_PATH");
    m_dbConfig.storagePath = envStoragePath.isEmpty() ? dbObj["storagePath"].toString() : envStoragePath;

    QJsonObject netObj = rootObj["network"].toObject();
    m_netConfig.bindAddress = netObj["bindAddress"].toString("127.0.0.1");
    m_netConfig.port = netObj["port"].toInt(8080);
    m_netConfig.serverSecret = netObj["serverSecret"].toString();
    m_netConfig.globalConnectionName = netObj["globalConnectionName"].toString();

    if (rootObj.contains("users") && rootObj.value("users").isArray()) {
        QJsonArray usersArr = rootObj.value("users").toArray();
        for (const auto& val : usersArr) {
            QJsonObject uObj = val.toObject();
            UserConfig u;
            u.id = uObj.value("id").toInt();
            u.username = uObj.value("username").toString();
            u.password = uObj.value("password").toString();
            QByteArray hash = QCryptographicHash::hash(u.password.toUtf8(), QCryptographicHash::Sha256);
            u.passwordHash = QString(hash.toHex());
            m_users.append(u);
        }
    }

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

network::NetworkConfig Config::networkConfig() const {
    return m_netConfig;
}

network::DatabaseConfig Config::databaseConfig() const {
    return m_dbConfig;
}

QString Config::authToken() const {
    return m_netConfig.serverSecret;
}

QList<UserConfig> Config::users() const {
    return m_users;
}

}
