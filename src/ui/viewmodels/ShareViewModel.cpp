// ShareViewModel.cpp
// 共享页 ViewModel 实现。

#include "ui/viewmodels/ShareViewModel.h"

#include "core/BeaconService.h"
#include "models/SharedFolder.h"

#include <QVariantMap>

namespace FengSui {

ShareViewModel::ShareViewModel(QObject* parent)
    : QObject(parent)
{
}

void ShareViewModel::bind(ShareService* share, BeaconService* beacon)
{
    if (m_share) {
        disconnect(m_share, nullptr, this, nullptr);
    }
    if (m_beacon) {
        disconnect(m_beacon, nullptr, this, nullptr);
    }
    m_share = share;
    m_beacon = beacon;

    if (m_share) {
        connect(m_share, &ShareService::sharedFoldersChanged,
                this, &ShareViewModel::refreshMyShares);
        connect(m_share, &ShareService::remoteShareListReceived,
                this, &ShareViewModel::onRemoteShareListReceived);
        connect(m_share, &ShareService::remoteShareItemsReceived,
                this, &ShareViewModel::onRemoteShareItemsReceived);
        connect(m_share, &ShareService::remoteShareDownloadStarted,
                this, &ShareViewModel::onDownloadStarted);
        connect(m_share, &ShareService::remoteShareDownloadCompleted,
                this, &ShareViewModel::onDownloadCompleted);
        connect(m_share, &ShareService::remoteShareFailed,
                this, &ShareViewModel::onRemoteShareFailed);
        connect(m_share, &ShareService::accessRequested,
                this, &ShareViewModel::onAccessRequested);
        refreshMyShares();
    }
    if (m_beacon) {
        connect(m_beacon, &BeaconService::peerOnline,
                this, &ShareViewModel::refreshSources);
        connect(m_beacon, &BeaconService::peerOffline,
                this, &ShareViewModel::refreshSources);
        refreshSources();
    }
}

void ShareViewModel::refreshMyShares()
{
    m_myShares.clear();
    if (m_share) {
        const QList<SharedFolder> folders = m_share->sharedFolders();
        for (const SharedFolder& f : folders) {
            QVariantMap m;
            m["shareId"] = f.shareId;
            m["localPath"] = f.localPath;
            m["displayName"] = f.displayName;
            m["isActive"] = f.isActive;
            m_myShares.append(m);
        }
    }
    emit mySharesChanged();
}

void ShareViewModel::refreshSources()
{
    m_sources.clear();
    if (m_beacon) {
        const QList<PeerInfo> peers = m_beacon->peers();
        for (const PeerInfo& p : peers) {
            if (!p.online || !p.shareEnabled) {
                continue;
            }
            QVariantMap m;
            m["peerId"] = p.peerId;
            m["displayName"] = p.displayName.isEmpty() ? p.peerId : p.displayName;
            m["endpoint"] = p.ip + QLatin1Char(':') + QString::number(p.port);
            m_sources.append(m);
        }
    }
    emit sourcesChanged();
}

std::optional<PeerInfo> ShareViewModel::currentSourcePeer() const
{
    if (!m_beacon || m_currentSourcePeerId.isEmpty()) {
        return std::nullopt;
    }
    const QList<PeerInfo> peers = m_beacon->peers();
    for (const PeerInfo& p : peers) {
        if (p.peerId == m_currentSourcePeerId) {
            return p;
        }
    }
    return std::nullopt;
}

void ShareViewModel::selectSource(const QString& peerId)
{
    m_currentSourcePeerId = peerId;
    m_currentShareId.clear();
    m_currentPath.clear();
    m_remoteShares.clear();
    m_remoteItems.clear();
    emit remoteSharesChanged();
    emit remoteItemsChanged();

    const auto peer = currentSourcePeer();
    if (peer && m_share) {
        setStatus(QStringLiteral("正在获取共享目录…"));
        m_share->requestShareList(*peer);
    }
}

void ShareViewModel::openShare(const QString& shareId)
{
    openPath(shareId, QStringLiteral("/"));
}

void ShareViewModel::openPath(const QString& shareId, const QString& path)
{
    const auto peer = currentSourcePeer();
    if (peer && m_share) {
        m_currentShareId = shareId;
        setStatus(QStringLiteral("正在打开…"));
        m_share->requestShareItems(*peer, shareId, path);
    }
}

void ShareViewModel::downloadItem(const QString& shareId, const QString& path)
{
    const auto peer = currentSourcePeer();
    if (peer && m_share) {
        setStatus(QStringLiteral("正在请求下载…"));
        m_share->requestShareDownload(*peer, shareId, path);
    }
}

void ShareViewModel::addSharedFolder(const QString& localPath)
{
    if (m_share) {
        m_share->addSharedFolder(localPath);
    }
}

void ShareViewModel::removeSharedFolder(const QString& shareId)
{
    if (m_share) {
        m_share->removeSharedFolder(shareId);
    }
}

void ShareViewModel::setSharedFolderActive(const QString& shareId, bool active)
{
    if (m_share) {
        m_share->setSharedFolderActive(shareId, active);
    }
}

void ShareViewModel::onRemoteShareListReceived(const QString& peerId, const QString&,
                                               const QList<RemoteSharedFolder>& shares)
{
    if (peerId != m_currentSourcePeerId) {
        return;
    }
    m_remoteShares.clear();
    for (const RemoteSharedFolder& s : shares) {
        QVariantMap m;
        m["shareId"] = s.shareId;
        m["displayName"] = s.displayName;
        m["fileCount"] = s.fileCount;
        m_remoteShares.append(m);
    }
    emit remoteSharesChanged();
    setStatus(shares.isEmpty() ? QStringLiteral("对方没有共享目录") : QString());
}

void ShareViewModel::onRemoteShareItemsReceived(const QString& peerId, const QString&,
                                                const QString& shareId, const QString& path,
                                                const QList<RemoteShareItem>& items)
{
    if (peerId != m_currentSourcePeerId) {
        return;
    }
    m_currentShareId = shareId;
    m_currentPath = path;
    m_remoteItems.clear();
    for (const RemoteShareItem& it : items) {
        QVariantMap m;
        m["name"] = it.name;
        m["path"] = it.path;
        m["size"] = it.size;
        m["isDir"] = it.isDir;
        m["modifiedAt"] = it.modifiedAt;
        m_remoteItems.append(m);
    }
    emit remoteItemsChanged();
    setStatus(QString());
}

void ShareViewModel::onDownloadStarted(const QString&, const QString& fileName,
                                       qint64, const QString&)
{
    setStatus(QStringLiteral("开始下载 %1").arg(fileName));
}

void ShareViewModel::onDownloadCompleted(const QString&, const QString& localPath)
{
    setStatus(QStringLiteral("下载完成：%1").arg(localPath));
}

void ShareViewModel::onRemoteShareFailed(const QString&, const QString& errorMessage)
{
    setStatus(QStringLiteral("失败：%1").arg(errorMessage));
}

void ShareViewModel::onAccessRequested(const QString& requestId, const QString& requesterName,
                                       const QString& deviceName, const QString& shareName)
{
    m_pendingAccessRequestId = requestId;
    emit accessRequested(requesterName, deviceName, shareName);
}

void ShareViewModel::resolveAccess(bool allowed, bool remember)
{
    if (m_share && !m_pendingAccessRequestId.isEmpty()) {
        m_share->resolveAccessRequest(m_pendingAccessRequestId, allowed, remember);
        m_pendingAccessRequestId.clear();
    }
}

void ShareViewModel::setStatus(const QString& msg)
{
    if (msg == m_statusMessage) {
        return;
    }
    m_statusMessage = msg;
    emit statusMessageChanged();
}

} // namespace FengSui
