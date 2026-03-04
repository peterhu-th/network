#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QThread>
#include "controller/AudioRecordController.h"

using namespace radar::network;

int main(int argc, char *argv[]) {
    // 创建 Qt 控制台应用的基础环境（网络和数据库模块都需要它）
    QCoreApplication app(argc, argv);

    qDebug() << "=== 后端独立集成测试 ===";
    {
        // 1. 设置测试用的路径为开发环境真实路径
        QString testDataPath = "./data";
        QDir().mkpath(testDataPath);
        qDebug() << "[1/4] 指向本地数据存储路径:" << testDataPath;

        // 2. 初始化 Controller
        AudioRecordController controller;

        // 伪造全局配置
        QVariantMap config;
        QVariantMap dbMap;
        dbMap["type"] = "QPSQL";
        dbMap["host"] = "127.0.0.1";
        dbMap["port"] = 5432;
        dbMap["name"] = "audio";
        dbMap["user"] = "postgres";
        dbMap["pass"] = "6";
        config["database"] = dbMap;

        QVariantMap netMap;
        netMap["port"] = 8080;
        config["network"] = netMap;

        QVariantMap storageMap;
        storageMap["path"] = testDataPath;
        config["storage"] = storageMap;

        auto initRes = controller.init(config);
        if (!initRes.isOk()) {
            qCritical() << "控制器初始化失败:" << initRes.errorMessage();
            return -1;
        }

        auto startRes = controller.start();
        if (!startRes.isOk()) {
            qCritical() << "控制器启动失败:" << startRes.errorMessage();
            return -1;
        }
        qDebug() << "[2/4] 后端服务已启动，监听 8080 端口.";

        QThread::msleep(500);

        // 3. 模拟前端发起 HTTP 请求
        qDebug() << "[3/4] 模拟前端客户端请求 /api/files...";
        QTcpSocket clientSocket;
        clientSocket.connectToHost("127.0.0.1", 8080);

        int timeout = 3000;
        while (clientSocket.state() != QAbstractSocket::ConnectedState && timeout > 0) {
            app.processEvents();
            QThread::msleep(10);
            timeout -= 10;
        }

        if (clientSocket.state() != QAbstractSocket::ConnectedState) {
            qCritical() << "无法连接到本地 HttpServer!";
            return -1;
        }

        // 构造标准的 HTTP GET 请求报文
        QByteArray request = "GET /api/files?limit=10 HTTP/1.1\r\n"
                             "Host: 127.0.0.1\r\n"
                             "Authorization: Bearer user\r\n"
                             "\r\n";
        clientSocket.write(request);
        timeout = 5000;

        while (clientSocket.state() == QAbstractSocket::ConnectedState && timeout > 0) {
            app.processEvents();
            QThread::msleep(10);
            timeout -= 10;
        }

        QByteArray response = clientSocket.readAll();
        if (response.isEmpty()) {
            qCritical() << "服务器未在规定时间内响应!";
        } else {
            qDebug() << "\n========== 收到的服务器响应 ==========";
            qDebug().noquote() << response;
            qDebug() << "======================================\n";
        }

        qDebug() << "[4/4] 测试结束，正在清理环境...";
        qDebug() << "=== 测试已转为后台服务模式，正在持续监听 8080 端口 ===";

        // 进入 Qt 事件循环，让程序永远挂起等待前端请求
        return app.exec();
    }
}