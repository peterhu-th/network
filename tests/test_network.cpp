#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QEventLoop>
#include <QDebug>
#include <iostream>
#include <QDateTime>
#include "NetworkService.h"

// Simple assertion macro
#define ASSERT_TRUE(cond) \
    if (!(cond)) { \
        qCritical() << "Assertion failed at" << __FILE__ << ":" << __LINE__; \
        exit(1); \
    }

class TestNetwork : public QObject {
    Q_OBJECT
public:
    explicit TestNetwork(QObject* parent = nullptr) : QObject(parent) {}

    void initTestCase() {
        m_testDir = QDir::currentPath() + "/test_data";
        QDir().mkpath(m_testDir);

        // Create dummy files
        createDummyFile("test1", 1024);
        createDummyFile("test2", 2048);

        m_dbName = m_testDir + "/test_audio_radar.db";
        
        // Config
        QVariantMap config;
        QVariantMap dbConfig;
        dbConfig["type"] = "QSQLITE";
        dbConfig["dbName"] = m_dbName;
        config["database"] = dbConfig;

        QVariantMap netConfig;
        netConfig["port"] = m_port;
        config["network"] = netConfig;

        QVariantMap storageConfig;
        storageConfig["path"] = m_testDir;
        config["storage"] = storageConfig;

        m_service = new radar::network::NetworkService(this);
        auto initRes = m_service->init(config);
        if (!initRes.isOk()) qCritical() << "Service init failed:" << initRes.errorMessage();
        ASSERT_TRUE(initRes.isOk());

        auto startRes = m_service->start();
        if (!startRes.isOk()) qCritical() << "Service start failed:" << startRes.errorMessage();
        ASSERT_TRUE(startRes.isOk());

        // Give some time for indexing
        wait(1000);
    }

    void cleanupTestCase() const {
        delete m_service;
        QDir(m_testDir).removeRecursively();
        QFile::remove(m_dbName);
    }

    void createDummyFile(const QString& name, int sizeBytes) const {
        QString wavPath = m_testDir + "/" + name + ".wav";
        QFile f(wavPath);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(QByteArray(sizeBytes, 'A'));
            f.close();
        }

        QString jsonPath = m_testDir + "/" + name + ".json";
        QJsonObject obj;
        obj["timestamp"] = QDateTime::currentMSecsSinceEpoch();
        obj["duration"] = 5000;
        QJsonDocument doc(obj);
        QFile fj(jsonPath);
        if (fj.open(QIODevice::WriteOnly)) {
            fj.write(doc.toJson());
            fj.close();
        }
    }

    void testApiList() const {
        qDebug() << "Running testApiList...";
        QNetworkAccessManager manager;
        qint64 currentMs = QDateTime::currentMSecsSinceEpoch();
        QString urlStr = QString("http://127.0.0.1:%1/api/files?startTime=0&endTime=%2")
                             .arg(m_port).arg(currentMs + 3600000);

        QNetworkRequest request((QUrl(urlStr)));
        QNetworkReply* reply = manager.get(request);

        waitForFinished(reply);

        ASSERT_TRUE(reply->error() == QNetworkReply::NoError);
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        ASSERT_TRUE(!doc.isNull());
        ASSERT_TRUE(doc.object()["code"].toInt() == 200);
        QJsonArray arr = doc.object()["data"].toArray();
        ASSERT_TRUE(arr.size() >= 2);
        
        delete reply;
        qDebug() << "testApiList Passed";
    }

    void testApiDownload() const {
        qDebug() << "Running testApiDownload...";
        // 1. Get List to get ID
        QNetworkAccessManager manager;
        qint64 currentMs = QDateTime::currentMSecsSinceEpoch();
        QString urlStr = QString("http://127.0.0.1:%1/api/files?startTime=0&endTime=%2")
                                     .arg(m_port).arg( currentMs + 3600000);
        QNetworkRequest requestList((QUrl(urlStr)));
        QNetworkReply* replyList = manager.get(requestList);
        waitForFinished(replyList);
        
        QJsonDocument doc = QJsonDocument::fromJson(replyList->readAll());
        QJsonArray arr = doc.object()["data"].toArray();
        ASSERT_TRUE(!arr.isEmpty());
        QString id = arr.first().toObject()["id"].toString();
        delete replyList;

        // 2. Download
        QNetworkRequest requestDownload(QUrl("http://127.0.0.1:" + QString::number(m_port) + "/api/download/" + id));
        requestDownload.setRawHeader("Authorization", "Bearer my_secret_token");
        QNetworkReply* replyDownload = manager.get(requestDownload);
        
        waitForFinished(replyDownload);

        ASSERT_TRUE(replyDownload->error() == QNetworkReply::NoError);
        QByteArray data = replyDownload->readAll();
        ASSERT_TRUE(data.size() > 0);
        // Should match dummy file size (1024 or 2048)
        ASSERT_TRUE(data.size() == 1024 || data.size() == 2048);
        
        delete replyDownload;
        qDebug() << "testApiDownload Passed";
    }

    static void wait(int ms) {
        QEventLoop loop;
        QTimer::singleShot(ms, &loop, &QEventLoop::quit);
        loop.exec();
    }

    static void waitForFinished(const QNetworkReply* reply) {
        if (reply->isFinished()) return;
        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
    }

private:
    radar::network::NetworkService* m_service{};
    QString m_testDir;
    QString m_dbName;
    int m_port = 8088;
};

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    
    TestNetwork test;
    test.initTestCase();
    test.testApiList();
    test.testApiDownload();
    test.cleanupTestCase();

    qDebug() << "All tests passed!";
    return 0;
}

#include "test_network.moc"
