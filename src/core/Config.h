#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QVariantMap>

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
    QVariantMap networkConfig() const;
    QVariantMap databaseConfig() const;
	QString authToken() const;

private:
    Config() = default;
    QVariantMap m_config;
};

}

#endif
