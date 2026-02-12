#include <QCoreApplication>
#include <QDir>
#include "Logger.h"
#include "Config.h"
#include "audio/AudioSourceFactory.h"
#include "processing/include/AudioProcessingService.h"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    LOG_INFO("Main", "AudioRadarClient started");

    auto& config = radar::Config::instance();
    const QStringList configCandidates = {
        QCoreApplication::applicationDirPath() + "/config.json",
        QDir::currentPath() + "/config/config.json",
        QDir::currentPath() + "/config.json"
    };
    bool configLoaded = false;
    for (const auto& path : configCandidates) {
        if (config.load(path)) {
            LOG_INFO("Config", QString("Loaded config: %1").arg(path));
            configLoaded = true;
            break;
        }
    }
    if (!configLoaded) {
        LOG_WARNING("Config", "Failed to load config. Using defaults.");
    }

    auto sourceResult = radar::audio::AudioSourceFactory::createAudioSource(&app);
    if (!sourceResult.isOk()) {
        LOG_ERROR("Audio", sourceResult.errorMessage());
        return -1;
    }
    auto* source = sourceResult.value();

    radar::AudioProcessingService processingService;

    QObject::connect(source, &radar::audio::IAudioSource::frameReady,
                     [&processingService](const radar::AudioFrame& frame) {
                         static int frameCount = 0;
                         ++frameCount;
                         auto result = processingService.processAudioFrame(frame);
                         if (!result.isOk()) {
                             LOG_ERROR("Processing", result.errorMessage());
                             return;
                         }
                         if (frameCount % 50 == 0) {
                             const auto& data = result.value();
                             LOG_INFO("Processing", QString("frame=%1 valid=%2 strength=%3 size=%4")
                                 .arg(frameCount)
                                 .arg(data.isValid ? "true" : "false")
                                 .arg(data.signalStrength, 0, 'f', 2)
                                 .arg(data.originalFrame.data.size()));
                         }
                     });

    QObject::connect(source, &radar::audio::IAudioSource::errorOccurred,
                     [](const QString& message, radar::ErrorCode code) {
                         LOG_ERROR("Audio", QString("error=%1 code=%2")
                             .arg(message)
                             .arg(static_cast<int>(code)));
                     });

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [source]() {
        if (source != nullptr) {
            source->stop();
        }
    });

    auto startResult = source->start();
    if (!startResult.isOk()) {
        LOG_ERROR("Audio", startResult.errorMessage());
        return -1;
    }

    return app.exec();
}
