// ShareViewModel.h
// 共享页 ViewModel：管理本机共享目录（增删/启停）与远程共享浏览下载。
// 远程列表以整快照形式经信号到达，用 QVariantList 暴露给 QML（非增量，无需 ListModel）。

#pragma once

#include "models/PeerInfo.h"
#include "core/ShareService.h"    // RemoteSharedFolder / RemoteShareItem 定义 + 信号签名

#include <QObject>
#include <QString>
#include <QVariantList>
#include <optional>

namespace FengSui {

class BeaconService;

class ShareViewModel : public QObject {
    Q_OBJECT

    // 我的共享目录：[{shareId,localPath,displayName,isActive}...]
    Q_PROPERTY(QVariantList myShares READ myShares NOTIFY mySharesChanged)
    // 可浏览的共享源（在线且开启共享的设备）：[{peerId,displayName,endpoint}...]
    Q_PROPERTY(QVariantList sources READ sources NOTIFY sourcesChanged)
    // 当前源的远程共享目录：[{shareId,displayName,fileCount}...]
    Q_PROPERTY(QVariantList remoteShares READ remoteShares NOTIFY remoteSharesChanged)
    // 当前浏览目录内容：[{name,path,size,isDir,modifiedAt}...]
    Q_PROPERTY(QVariantList remoteItems READ remoteItems NOTIFY remoteItemsChanged)
    // 当前浏览路径（面包屑）
    Q_PROPERTY(QString currentPath READ currentPath NOTIFY remoteItemsChanged)
    // 状态/提示文案
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit ShareViewModel(QObject* parent = nullptr);

    void bind(ShareService* share, BeaconService* beacon);

    QVariantList myShares() const { return m_myShares; }
    QVariantList sources() const { return m_sources; }
    QVariantList remoteShares() const { return m_remoteShares; }
    QVariantList remoteItems() const { return m_remoteItems; }
    QString currentPath() const { return m_currentPath; }
    QString statusMessage() const { return m_statusMessage; }

    // 本机共享管理
    Q_INVOKABLE void addSharedFolder(const QString& localPath);
    Q_INVOKABLE void removeSharedFolder(const QString& shareId);
    Q_INVOKABLE void setSharedFolderActive(const QString& shareId, bool active);

    // 远程浏览
    Q_INVOKABLE void selectSource(const QString& peerId);
    Q_INVOKABLE void openShare(const QString& shareId);
    Q_INVOKABLE void openPath(const QString& shareId, const QString& path);
    Q_INVOKABLE void downloadItem(const QString& shareId, const QString& path);

    // 响应访问授权弹窗
    Q_INVOKABLE void resolveAccess(bool allowed, bool remember);

signals:
    void mySharesChanged();
    void sourcesChanged();
    void remoteSharesChanged();
    void remoteItemsChanged();
    void statusMessageChanged();
    // 收到需授权的访问请求（QML 弹出授权对话框）
    void accessRequested(const QString& requesterName,
                         const QString& deviceName,
                         const QString& shareName);

private slots:
    void refreshMyShares();
    void refreshSources();
    void onRemoteShareListReceived(const QString& peerId, const QString& requestId,
                                   const QList<RemoteSharedFolder>& shares);
    void onRemoteShareItemsReceived(const QString& peerId, const QString& requestId,
                                    const QString& shareId, const QString& path,
                                    const QList<RemoteShareItem>& items);
    void onDownloadStarted(const QString& downloadId, const QString& fileName,
                           qint64 fileSize, const QString& localPath);
    void onDownloadCompleted(const QString& downloadId, const QString& localPath);
    void onRemoteShareFailed(const QString& requestId, const QString& errorMessage);
    void onAccessRequested(const QString& requestId, const QString& requesterName,
                           const QString& deviceName, const QString& shareName);

private:
    void setStatus(const QString& msg);
    std::optional<PeerInfo> currentSourcePeer() const;

    ShareService*  m_share = nullptr;
    BeaconService* m_beacon = nullptr;

    QVariantList m_myShares;
    QVariantList m_sources;
    QVariantList m_remoteShares;
    QVariantList m_remoteItems;
    QString      m_currentSourcePeerId;
    QString      m_currentShareId;
    QString      m_currentPath;
    QString      m_statusMessage;
    QString      m_pendingAccessRequestId;   // 待响应的授权请求 ID
};

} // namespace FengSui
