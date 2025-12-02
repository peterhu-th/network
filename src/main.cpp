#include <QCoreApplication>
#include "Logger.h"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    LOG_INFO("Main", "AudioRadarClient started");
    return app.exec();
}