#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
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
    m_dbConfig.username = dbObj["username"].toString("postgres");
    const QString envDbPassword = qEnvironmentVariable("AUDIO_DB_PASSWORD");
    m_dbConfig.passWord = envDbPassword.isEmpty() ? dbObj["passWord"].toString() : envDbPassword;
    const QString envStoragePath = qEnvironmentVariable("AUDIO_STORAGE_PATH");
    m_dbConfig.storagePath = envStoragePath.isEmpty() ? dbObj["storagePath"].toString("./data") : envStoragePath;

    QJsonObject netObj = rootObj["network"].toObject();
    m_netConfig.bindAddress = netObj["bindAddress"].toString("127.0.0.1");
    m_netConfig.port = netObj["port"].toInt(8080);
    const QString envServerSecret = qEnvironmentVariable("AUDIO_SERVER_SECRET");
    m_netConfig.serverSecret = envServerSecret.isEmpty()
        ? netObj["serverSecret"].toString("RADAR_DEV_SECRET_CHANGE_ME")
        : envServerSecret;
    m_netConfig.globalConnectionName = netObj["globalConnectionName"].toString("Audio_GlobalPool");

    QJsonObject smtpObj = rootObj["smtp"].toObject();
    m_smtpConfig.server = smtpObj["server"].toString("smtp.qq.com");
    m_smtpConfig.port = smtpObj["port"].toInt(465);
    m_smtpConfig.username = smtpObj["username"].toString();
    const QString envSmtpPassword = qEnvironmentVariable("AUDIO_SMTP_PASSWORD");
    m_smtpConfig.password = envSmtpPassword.isEmpty() ? smtpObj["password"].toString() : envSmtpPassword;

    QJsonObject adminObj = rootObj["admin"].toObject();
    const QString envAdminEmail = qEnvironmentVariable("AUDIO_ADMIN_EMAIL");
    const QString envAdminPassword = qEnvironmentVariable("AUDIO_ADMIN_PASSWORD");
    m_adminEmail = envAdminEmail.isEmpty() ? adminObj["email"].toString() : envAdminEmail;
    m_adminPassword = envAdminPassword.isEmpty() ? adminObj["password"].toString() : envAdminPassword;

    QJsonObject processingObj = rootObj["processing"].toObject();
    m_denoiseConfig.lowCutoff = processingObj["lowCutoff"].toDouble(2000.0);
    m_denoiseConfig.highCutoff = processingObj["highCutoff"].toDouble(19000.0);
    m_denoiseConfig.fftSize = processingObj["fftSize"].toInt(8192);
    m_denoiseConfig.hopSize = processingObj["hopSize"].toInt(2048);
    m_denoiseConfig.aleMu1 = processingObj["aleMu1"].toDouble(0.0005);
    m_denoiseConfig.aleM1 = processingObj["aleM1"].toInt(16);
    m_denoiseConfig.aleDelta1 = processingObj["aleDelta1"].toInt(3);
    m_denoiseConfig.aleMu2 = processingObj["aleMu2"].toDouble(0.001);
    m_denoiseConfig.aleM2 = processingObj["aleM2"].toInt(32);
    m_denoiseConfig.aleDelta2 = processingObj["aleDelta2"].toInt(5);
    m_denoiseConfig.aleMixRatio = processingObj["aleMixRatio"].toDouble(0.7);
    m_denoiseConfig.aleSignalGain = processingObj["aleSignalGain"].toDouble(1.3);
    m_denoiseConfig.sadThreshold = processingObj["sadThreshold"].toDouble(1e-3);

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

DenoiseConfig Config::denoiseConfig() const {
    return m_denoiseConfig;
}

network::SmtpConfig Config::smtpConfig() const {
    return m_smtpConfig;
}

QString Config::adminEmail() const {
    return m_adminEmail;
}

QString Config::adminPassword() const {
    return m_adminPassword;
}

QString Config::authToken() const {
    return m_netConfig.serverSecret;
}

}
