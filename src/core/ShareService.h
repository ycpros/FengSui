// ShareService.h
// 共享目录服务：管理本机共享目录，并处理远程浏览、下载和访问授权。

#pragma once

#include "models/DownloadLog.h"
#include "models/PeerInfo.h"
#include "models/SharedFolder.h"

#include <QByteArray>
#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QStringList>
#include <optional>

class QCryptographicHash;
class QFile;
namespace FengSui {

class AccessGrantRepository;
class AppSettings;
class DownloadLogRepository;
class ShareRepository;
class SignalService;
class TcpConnection;

// 远程共享目录摘要。
struct RemoteSharedFolder {
    QString peerId;       // 来源设备 peerId。
    QString shareId;      // 远程共享目录 ID。
    QString displayName;  // 远程显示名称。
    int fileCount = 0;    // 远程估算文件数量。
};

// 远程共享目录项。
struct RemoteShareItem {
    QString peerId;      // 来源设备 peerId。
    QString shareId;     // 来源共享目录 ID。
    QString path;        // 项目相对共享根目录的路径。
    QString name;        // 展示名称。
    qint64 size = 0;     // 文件大小；目录为 0。
    QString modifiedAt;  // ISO 时间字符串。
    bool isDir = false;  // 是否为目录。
};

// 共享目录业务服务。
// 负责本机共享目录的路径校验、去重、启停、远程浏览下载和访问授权。
class ShareService : public QObject {
    Q_OBJECT

public:
    explicit ShareService(QObject* parent = nullptr);
    ~ShareService() override;

    void setShareRepository(ShareRepository* repository);
    void setSignalService(SignalService* service);
    void setAppSettings(AppSettings* settings);
    void setDownloadLogRepository(DownloadLogRepository* repository);
    void setAccessGrantRepository(AccessGrantRepository* repository);

    std::optional<SharedFolder> addSharedFolder(
        const QString& localPath,
        const QString& displayName = QString());
    bool removeSharedFolder(const QString& shareId);
    bool setSharedFolderActive(const QString& shareId, bool active);

    QList<SharedFolder> sharedFolders() const;
    QList<SharedFolder> activeSharedFolders() const;
    bool hasActiveShares() const;

    // 请求远程设备的共享目录列表。
    // 返回 request_id；失败时仍返回 ID 并通过 remoteShareFailed 通知。
    QString requestShareList(const PeerInfo& peer);

    // 请求远程共享目录下的项目列表。
    QString requestShareItems(const PeerInfo& peer,
                              const QString& shareId,
                              const QString& path);

    // 请求下载远程共享文件。
    QString requestShareDownload(const PeerInfo& peer,
                                 const QString& shareId,
                                 const QString& path);

public slots:
    // 处理 share.* JSON 消息，由 SignalService 转发。
    void handleShareMessage(TcpConnection* connection,
                            const QJsonObject& message);

    // 处理共享下载二进制块，由 SignalService 转发。
    void handleDownloadChunk(const QString& downloadId,
                             quint32 chunkIndex,
                             const QByteArray& data);

    // 响应访问授权弹窗。
    void resolveAccessRequest(const QString& requestId,
                              bool allowed,
                              bool remember);

signals:
    void sharedFoldersChanged();
    void shareAvailabilityChanged(bool enabled);
    void remoteShareListReceived(const QString& peerId,
                                 const QString& requestId,
                                 const QList<RemoteSharedFolder>& shares);
    void remoteShareItemsReceived(const QString& peerId,
                                  const QString& requestId,
                                  const QString& shareId,
                                  const QString& path,
                                  const QList<RemoteShareItem>& items);
    void remoteShareDownloadStarted(const QString& downloadId,
                                    const QString& fileName,
                                    qint64 fileSize,
                                    const QString& localPath);
    void remoteShareDownloadProgress(const QString& downloadId,
                                     qint64 bytesTransferred,
                                     qint64 totalBytes);
    void remoteShareDownloadCompleted(const QString& downloadId,
                                      const QString& localPath);
    void remoteShareFailed(const QString& requestId,
                           const QString& errorMessage);
    void accessRequested(const QString& requestId,
                         const QString& requesterName,
                         const QString& deviceName,
                         const QString& shareName);
    void outboundShareMessage(const PeerInfo& peer,
                              const QJsonObject& message);

private:
    struct PendingAccessRequest {
        QString requestId;              // share 请求 ID 或 download_id。
        QString peerId;                 // 请求方 peerId。
        QString requesterName;          // 弹窗展示名称。
        QString deviceName;             // 弹窗设备名。
        QString shareName;              // 弹窗目标共享名称。
        QStringList shareIds;           // 本次授权覆盖的共享目录 ID。
        QJsonObject message;            // 原始请求，授权后继续处理。
        QPointer<TcpConnection> connection; // 原请求连接，可能在等待授权期间断开。
    };

    struct DownloadContext {
        QString downloadId;             // sd_ 开头的下载 ID。
        PeerInfo peer;                  // 对端设备信息。
        QString shareId;                // 来源共享目录。
        QString remotePath;             // 远程相对路径。
        QString localPath;              // 本地保存路径或发送端源文件路径。
        QString fileName;               // 文件名。
        QString sha256;                 // 远端声明或本地计算 SHA-256。
        qint64 fileSize = 0;            // 总字节数。
        qint64 transferredBytes = 0;    // 已处理字节数。
        quint32 nextChunkIndex = 0;     // 下一块序号。
        bool outgoing = false;          // true=本机向对端发送共享文件。
        QFile* file = nullptr;          // 文件句柄，按 outgoing 决定读/写。
        QCryptographicHash* hasher = nullptr; // 接收端 SHA-256 累加器。
        QPointer<TcpConnection> connection;   // 当前 TCP 连接。
    };

    static QString normalizePath(const QString& localPath);
    static QString defaultDisplayName(const QString& normalizedPath);
    std::optional<SharedFolder> findByPath(const QString& normalizedPath) const;
    void emitChangedIfNeeded(bool previousAvailability);
    QString localPeerId() const;
    QString generateRequestId(const QString& prefix) const;
    QString accessKey(const QString& peerId, const QString& shareId) const;
    bool hasAccess(const QString& peerId, const QString& shareId) const;
    bool ensureAccessOrDefer(TcpConnection* connection,
                             const QJsonObject& message,
                             const QStringList& shareIds,
                             const QString& shareName);
    void continueAuthorizedRequest(const PendingAccessRequest& request);
    void sendShareError(TcpConnection* connection,
                        const QString& requestId,
                        const QString& toPeerId,
                        const QString& errorCode,
                        const QString& errorMessage);
    void handleShareListRequest(TcpConnection* connection,
                                const QJsonObject& message);
    void handleShareItemsRequest(TcpConnection* connection,
                                 const QJsonObject& message);
    void handleShareDownloadRequest(TcpConnection* connection,
                                    const QJsonObject& message);
    void handleShareListReply(const QJsonObject& message);
    void handleShareItemsReply(const QJsonObject& message);
    void handleShareDownloadReply(TcpConnection* connection,
                                  const QJsonObject& message);
    void handleShareDownloadComplete(const QJsonObject& message);
    void handleShareError(const QJsonObject& message);
    QList<RemoteSharedFolder> parseRemoteShares(const QString& peerId,
                                                const QJsonObject& message) const;
    QList<RemoteShareItem> parseRemoteItems(const QString& peerId,
                                            const QString& shareId,
                                            const QJsonObject& message) const;
    QJsonArray buildLocalShareList() const;
    QJsonArray buildLocalItems(const SharedFolder& folder,
                               const QString& remotePath,
                               QString& errorOut) const;
    std::optional<SharedFolder> activeShareById(const QString& shareId) const;
    QString safeLocalPath(const SharedFolder& folder,
                          const QString& remotePath,
                          bool requireFile,
                          QString& errorOut) const;
    QString uniqueDownloadPath(const QString& fileName) const;
    void startOutgoingDownload(const QString& downloadId);
    void sendNextDownloadChunk(const QString& downloadId);
    void finishDownload(const QString& downloadId,
                        bool success,
                        const QString& errorMessage = QString());
    void saveDownloadLog(const DownloadContext& context, bool success);
    static QString computeFileSha256(const QString& filePath);

    ShareRepository* m_repository = nullptr;
    SignalService* m_signalService = nullptr;
    AppSettings* m_settings = nullptr;
    DownloadLogRepository* m_downloadLogRepository = nullptr;
    AccessGrantRepository* m_accessGrantRepository = nullptr;
    QHash<QString, PendingAccessRequest> m_pendingAccessRequests;
    QHash<QString, DownloadContext> m_downloads;
    QSet<QString> m_sessionAccessGrants;
};

} // namespace FengSui

Q_DECLARE_METATYPE(FengSui::RemoteSharedFolder)
Q_DECLARE_METATYPE(FengSui::RemoteShareItem)
