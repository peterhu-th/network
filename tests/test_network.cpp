#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QDebug>
#include "controller/AudioRecordController.h"

// 【新增】：引入底层的 Config 单例
#include "../core/Config.h"

using namespace radar::network;

int main(int argc, char *argv[]) {
    // 创建 Qt 控制台应用的基础环境（网络和数据库模块都需要它依赖的事件循环）
    QCoreApplication app(argc, argv);

    qDebug() << "======================================";
    qDebug() << "    Audio Radar Backend 启动序列      ";
    qDebug() << "======================================";

    // --- 【新增】：0. 定位并加载唯一的配置文件 ---
    qDebug() << "[0/3] 正在加载全局配置文件...";
    auto& config = radar::Config::instance();

    // 兼容终端直接运行和 CLion Debug 运行的路径寻址
    QString configPath = QCoreApplication::applicationDirPath() + "/../../config/config.json";

    if (!config.load(configPath)) {
        qCritical() << "❌ 无法读取 config.json! 请检查路径:" << configPath;
        return -1; // 快速失败
    }
    qDebug() << "✅ 配置加载成功.";
    // ---------------------------------------------


    // 1. 初始化并启动 Controller
    AudioRecordController controller;

    qDebug() << "[1/3] 正在初始化网络控制器...";
    // 【修改】：传入解析好的强类型 DTO 结构体
    auto initRes = controller.init(config.databaseConfig(), config.networkConfig());
    if (!initRes.isOk()) {
        qCritical() << "❌ 控制器初始化致命错误:" << initRes.errorMessage();
        return -1;
    }
    qDebug() << "✅ 初始化成功.";

    qDebug() << "[2/3] 正在绑定端口并启动底层监听服务...";
    auto startRes = controller.start();
    if (!startRes.isOk()) {
        qCritical() << "❌ 控制器启动致命错误:" << startRes.errorMessage();
        return -1;
    }
    qDebug() << "✅ 底层服务已启动.";

    qDebug() << "[3/3] 正在启动文件检索引... (后台运行)";

    qDebug() << "\n======================================";
    qDebug() << "🚀 后端引擎已全速运行，持续监听" << config.networkConfig().port << "端口";
    qDebug() << "⏳ 正在等待前端 (Vue) 接入...";
    qDebug() << "======================================\n";

    return QCoreApplication::exec();
}