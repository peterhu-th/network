#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QVariantMap>
#include <QList>
#include "Types.h"
#include "../network/NetworkDTO.h"

namespace radar {

class Config {
public:
    static Config& instance();

    bool load(const QString& path);

    QString getString(const QString& key, const QString& defaultValue = {}) const;
    int getInt(const QString& key, int defaultValue = 0) const;
    bool getBool(const QString& key, bool defaultValue = false) const;

    QVariantMap audioConfig() const;
    QVariantMap storageConfig() const;
    network::NetworkConfig networkConfig() const;
    network::DatabaseConfig databaseConfig() const;
	QString authToken() const;
    QList<UserConfig> users() const;

private:
    Config() = default;
    QVariantMap m_config;
    network::DatabaseConfig m_dbConfig;
    network::NetworkConfig m_netConfig;
    QList<UserConfig> m_users;
};

}

#endif
