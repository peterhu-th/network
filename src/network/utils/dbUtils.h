#ifndef DB_UTILS_H
#define DB_UTILS_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QThreadStorage>
#include <QUuid>

namespace radar::network::utils {
    // 封装线程局部数据库连接上下文，利用 RAII 机制管理连接生命周期
    struct ThreadLocalDatabase {
        QString connName;

        explicit ThreadLocalDatabase(const QString& baseConnectionName) {
            connName = baseConnectionName + "_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
            QSqlDatabase baseDb = QSqlDatabase::database(baseConnectionName);

            if (!baseDb.isValid()) {
                qWarning() << "Base database connection is invalid!";
                return;
            }

            QSqlDatabase threadDb = QSqlDatabase::cloneDatabase(baseDb, connName);
            if (!threadDb.open()) {
                qWarning() << "Failed to open thread local database:" << threadDb.lastError().text();
            }
        }

        ~ThreadLocalDatabase() {
            if (QSqlDatabase::contains(connName)) {
                {
                    QSqlDatabase db = QSqlDatabase::database(connName);
                    if (db.isOpen()) {
                        db.close();
                    }
                }
                QSqlDatabase::removeDatabase(connName);
            }
        }
    };

    class DbUtils {
    public:
        static QSqlDatabase getConnection(const QString& baseConnectionName) {
            static QThreadStorage<std::shared_ptr<ThreadLocalDatabase>> tls;
            if (!tls.hasLocalData()) {
                tls.setLocalData(std::make_shared<ThreadLocalDatabase>(baseConnectionName));
            }
            QSqlDatabase db = QSqlDatabase::database(tls.localData()->connName);
            if (!db.isOpen()) {
                db.open();
            }
            return db;
        }
    };
}

#endif