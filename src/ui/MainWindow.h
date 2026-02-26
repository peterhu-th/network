#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QStandardItemModel>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace radar::ui {

    class MainWindow : public QWidget {
        Q_OBJECT
    public:
        explicit MainWindow(QWidget* parent = nullptr);
        ~MainWindow() override;

        void loadFiles();

    private slots:
        void onPrevPage();
        void onNextPage();
        void onDownloadFiles();
        void onFilesLoaded(QNetworkReply* reply);
        void onDownloadFinished(QNetworkReply* reply, const QString& savePath);

    private:
        QTableView* m_tableView;
        QStandardItemModel* m_model;
        QPushButton* m_btnPrev;
        QPushButton* m_btnNext;
        QPushButton* m_btnDownload;
        QLabel* m_lblPage;
        
        QNetworkAccessManager* m_networkManager;
        int m_limit = 20;
        int m_offset = 0;
        int m_total = 0;
        
        void setupUi();
    };

}

#endif // MAINWINDOW_H
