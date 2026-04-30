#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QVariantMap>
#include "../network/NetworkDTO.h"

namespace radar {

struct DenoiseConfig {
    double lowCutoff = 2000.0;
    double highCutoff = 19000.0;
    int fftSize = 8192;
    int hopSize = 2048;
    double aleMu1 = 0.0005;
    int aleM1 = 16;
    int aleDelta1 = 3;
    double aleMu2 = 0.001;
    int aleM2 = 32;
    int aleDelta2 = 5;
    double aleMixRatio = 0.7;
    double aleSignalGain = 1.3;
    double sadThreshold = 1e-3;
};

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
    DenoiseConfig denoiseConfig() const;
	QString authToken() const;

private:
    Config() = default;
    QVariantMap m_config;
    network::DatabaseConfig m_dbConfig;
    network::NetworkConfig m_netConfig;
    DenoiseConfig m_denoiseConfig;
};

}

#endif
