#include <QCoreApplication>
#include <QTest>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSignalSpy>
#include "NetworkService.h"
#include "DatabaseManager.h"

class TestNetwork : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    void testIndexing();
    void testApiList();
    void testApiDownload();

private:
    radar::network::NetworkService* m_service;
    QString m_testDir;
    QString m_dbName;
    int m_port = 8088;

    void createDummyFile(const QString& name, int sizeBytes);
};

void TestNetwork::initTestCase() {
    m_testDir = QDir::currentPath() + "/test_data";
    QDir().mkpath(m_testDir);

    // Create dummy files
    createDummyFile("test1", 1024);
    createDummyFile("test2", 2048);

    m_dbName = "test_audio_radar.db";
    
    // Config
    QVariantMap config;
    QVariantMap dbConfig;
    dbConfig["type"] = "QSQLITE"; // Use SQLite for testing
    dbConfig["name"] = m_dbName;
    config["db"] = dbConfig;

    QVariantMap netConfig;
    netConfig["port"] = m_port;
    config["network"] = netConfig;

    QVariantMap storageConfig;
    storageConfig["path"] = m_testDir;
    config["storage"] = storageConfig;

    m_service = new radar::network::NetworkService(this);
    QVERIFY(m_service->init(config));
    QVERIFY(m_service->start());

    // Give some time for indexing
    QTest::qWait(1000);
}

void TestNetwork::cleanupTestCase() {
    delete m_service;
    QDir(m_testDir).removeRecursively();
    QFile::remove(m_dbName);
}

void TestNetwork::createDummyFile(const QString& name, int sizeBytes) {
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

void TestNetwork::testIndexing() {
    // We cannot easily check internal state of NetworkService, but we can check via API or DB directly if we had access
    // Here we rely on API test
}

void TestNetwork::testApiList() {
    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl("http://127.0.0.1:" + QString::number(m_port) + "/api/files"));
    QNetworkReply* reply = manager.get(request);
    
    QSignalSpy spy(reply, &QNetworkReply::finished);
    QVERIFY(spy.wait(2000));

    QVERIFY(reply->error() == QNetworkReply::NoError);
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QVERIFY(!doc.isNull());
    QVERIFY(doc.object()["code"].toInt() == 200);
    QJsonArray arr = doc.object()["data"].toArray();
    QVERIFY(arr.size() >= 2);
    
    delete reply;
}

void TestNetwork::testApiDownload() {
    // 1. Get List to get ID
    QNetworkAccessManager manager;
    QNetworkRequest requestList(QUrl("http://127.0.0.1:" + QString::number(m_port) + "/api/files"));
    QNetworkReply* replyList = manager.get(requestList);
    QSignalSpy spyList(replyList, &QNetworkReply::finished);
    spyList.wait(1000);
    
    QJsonDocument doc = QJsonDocument::fromJson(replyList->readAll());
    QJsonArray arr = doc.object()["data"].toArray();
    QVERIFY(arr.size() > 0);
    QString id = arr.first().toObject()["id"].toString();
    delete replyList;

    // 2. Download
    QNetworkRequest requestDownload(QUrl("http://127.0.0.1:" + QString::number(m_port) + "/api/download/" + id));
    QNetworkReply* replyDownload = manager.get(requestDownload);
    
    QSignalSpy spyDownload(replyDownload, &QNetworkReply::finished);
    QVERIFY(spyDownload.wait(2000));

    QVERIFY(replyDownload->error() == QNetworkReply::NoError);
    QByteArray data = replyDownload->readAll();
    QVERIFY(data.size() > 0);
    // Should match dummy file size (1024 or 2048)
    QVERIFY(data.size() == 1024 || data.size() == 2048);
    
    delete replyDownload;
}

QTEST_MAIN(TestNetwork)
#include "test_network.moc"
