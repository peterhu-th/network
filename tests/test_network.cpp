#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QDebug>
#include "controller/AudioRecordController.h"
#include "../core/Config.h"

using namespace radar::network;

int main(int argc, char *argv[]) {
    // 创建 Qt 控制台应用的基础环境
    QCoreApplication app(argc, argv);

    // 定位并加载配置文件
    auto& config = radar::Config::instance();
    QString configPath = QCoreApplication::applicationDirPath() + "/../../config/config.json";
    if (!config.load(configPath)) {
        qCritical() << "无法读取 config，请检查路径:" << configPath;
        return -1;
    }
    // 初始化并启动 Controller
    AudioRecordController controller;
    auto initRes = controller.init(config.databaseConfig(), config.networkConfig());
    if (!initRes.isOk()) {
        qCritical() << "控制器初始化错误:" << initRes.errorMessage();
        return -1;
    }
    auto startRes = controller.start();
    if (!startRes.isOk()) {
        qCritical() << "控制器启动错误:" << startRes.errorMessage();
        return -1;
    }
    qDebug() << "持续监听" << config.networkConfig().port << "端口";

    return QCoreApplication::exec();
}