#include "MainWindow.h"
#include <QHeaderView>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QDebug>

namespace radar::ui {

    MainWindow::MainWindow(QWidget* parent) : QWidget(parent) {
        setupUi();
        m_networkManager = new QNetworkAccessManager(this);
        loadFiles();
    }

    MainWindow::~MainWindow() {
    }

    void MainWindow::setupUi() {
        setWindowTitle("Audio Radar Client");
        resize(800, 600);

        auto* mainLayout = new QVBoxLayout(this);

        m_tableView = new QTableView(this);
        m_model = new QStandardItemModel(0, 4, this);
        m_model->setHeaderData(0, Qt::Horizontal, "ID");
        m_model->setHeaderData(1, Qt::Horizontal, "File Path");
        m_model->setHeaderData(2, Qt::Horizontal, "Created At");
        m_model->setHeaderData(3, Qt::Horizontal, "Size (Bytes)");
        
        m_tableView->setModel(m_model);
        m_tableView->horizontalHeader()->setStretchLastSection(true);
        m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        mainLayout->addWidget(m_tableView);

        auto* bottomLayout = new QHBoxLayout();
        m_btnPrev = new QPushButton("Previous Page", this);
        m_btnNext = new QPushButton("Next Page", this);
        m_lblPage = new QLabel("Page: 1", this);
        m_btnDownload = new QPushButton("Download Selected", this);

        bottomLayout->addWidget(m_btnPrev);
        bottomLayout->addWidget(m_lblPage);
        bottomLayout->addWidget(m_btnNext);
        bottomLayout->addStretch();
        bottomLayout->addWidget(m_btnDownload);
        
        mainLayout->addLayout(bottomLayout);

        connect(m_btnPrev, &QPushButton::clicked, this, &MainWindow::onPrevPage);
        connect(m_btnNext, &QPushButton::clicked, this, &MainWindow::onNextPage);
        connect(m_btnDownload, &QPushButton::clicked, this, &MainWindow::onDownloadFiles);
    }

    void MainWindow::loadFiles() {
        QUrl url("http://127.0.0.1:8080/api/files");
        QUrlQuery query;
        query.addQueryItem("limit", QString::number(m_limit));
        query.addQueryItem("offset", QString::number(m_offset));
        url.setQuery(query);

        QNetworkRequest request(url);
        QNetworkReply* reply = m_networkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            onFilesLoaded(reply);
        });
    }

    void MainWindow::onFilesLoaded(QNetworkReply* reply) {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(this, "Network Error", reply->errorString());
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) return;

        QJsonObject obj = doc.object();
        if (obj["code"].toInt() != 200) {
            return;
        }

        m_total = obj["total"].toInt();
        int currentPage = (m_offset / m_limit) + 1;
        int totalPages = (m_total + m_limit - 1) / m_limit;
        m_lblPage->setText(QString("Page: %1 / %2").arg(currentPage).arg(std::max(1, totalPages)));
        
        m_btnPrev->setEnabled(m_offset > 0);
        m_btnNext->setEnabled(m_offset + m_limit < m_total);

        m_model->removeRows(0, m_model->rowCount());
        QJsonArray arr = obj["data"].toArray();
        for (int i = 0; i < arr.size(); ++i) {
            QJsonObject item = arr[i].toObject();
            QList<QStandardItem*> row;
            row << new QStandardItem(item["id"].toString());
            row << new QStandardItem(item["path"].toString());
            row << new QStandardItem(item["created_at"].toString());
            row << new QStandardItem(QString::number(item["size"].toInt()));
            m_model->appendRow(row);
        }
    }

    void MainWindow::onPrevPage() {
        if (m_offset >= m_limit) {
            m_offset -= m_limit;
            loadFiles();
        }
    }

    void MainWindow::onNextPage() {
        if (m_offset + m_limit < m_total) {
            m_offset += m_limit;
            loadFiles();
        }
    }

    void MainWindow::onDownloadFiles() {
        QModelIndexList selection = m_tableView->selectionModel()->selectedRows();
        if (selection.isEmpty()) {
            QMessageBox::information(this, "Info", "Please select a file to download.");
            return;
        }

        QString id = m_model->item(selection[0].row(), 0)->text();
        QString originalPath = m_model->item(selection[0].row(), 1)->text();
        QFileInfo fi(originalPath);
        QString defaultName = fi.fileName().isEmpty() ? "download.wav" : fi.fileName();

        QString savePath = QFileDialog::getSaveFileName(this, "Save File", defaultName, "Audio Files (*.wav);;All Files (*)");
        if (savePath.isEmpty()) return;

        QUrl url(QString("http://127.0.0.1:8080/api/download/%1").arg(id));
        QNetworkRequest request(url);
        // 原来的下载接口加上了简单的鉴权 Header:
        request.setRawHeader("authorization", "Bearer my_secret_token");
        
        QNetworkReply* reply = m_networkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, savePath]() {
            onDownloadFinished(reply, savePath);
        });
    }

    void MainWindow::onDownloadFinished(QNetworkReply* reply, const QString& savePath) {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(this, "Download Error", reply->errorString());
            return;
        }

        QFile file(savePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
            file.close();
            QMessageBox::information(this, "Success", "File downloaded successfully!");
        } else {
            QMessageBox::warning(this, "File Error", "Failed to save file locally.");
        }
    }
}
