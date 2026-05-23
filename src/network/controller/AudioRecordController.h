#ifndef AUDIO_RECORD_CONTROLLER_H
#define AUDIO_RECORD_CONTROLLER_H

#include <memory>
#include "../service/AudioRecordService.h"
#include "../NetworkDTO.h"
#include "AuthController.h"
#include "AdminController.h"
#include "../mapper/UserMapper.h"
#include "../utils/SmtpClient.h"

namespace radar::network {
    class AudioRecordController : public QObject {
        Q_OBJECT

    public:
        explicit AudioRecordController(QObject *parent = nullptr);
        ~AudioRecordController() override;
        Result<void> init(const DatabaseConfig& dbConfig, const NetworkConfig& netConfig);
        [[nodiscard]] Result<void> start();
        static void stop();

    private:
        std::unique_ptr<AudioRecordService> m_service;
        std::shared_ptr<AuthService> m_authService;
        std::shared_ptr<AdminService> m_adminService;
        std::shared_ptr<UserMapper> m_userMapper;
        std::shared_ptr<SmtpClient> m_smtpClient;

        std::unique_ptr<QHttpServer> m_httpServer;
        std::unique_ptr<AuthController> m_authController;
        std::unique_ptr<AdminController> m_adminController;
        int m_port = 8080;
        QHostAddress m_bindAddress = QHostAddress::LocalHost;
        QString m_jwtSecret;

        void setupRoutes();
        [[nodiscard]] QHttpServerResponse handleListFiles(const QHttpServerRequest& request) const;
        [[nodiscard]] QHttpServerResponse handleDeleteFile(const QString& idStr, const QHttpServerRequest& request) const;
        void handleDownload(const QHttpServerRequest& request, QHttpServerResponder& responder);
        [[nodiscard]] QHttpServerResponse handleBatchDownloadJob(const QHttpServerRequest& request) const;  // 批量下载任务
        void handleBatchDownloadFile(const QHttpServerRequest& request, QHttpServerResponder& responder, const QString& taskId);
    };
}
#endif
