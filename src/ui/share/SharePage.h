// SharePage.h
// 共享文件页面：浏览在线共享、下载文件、管理我的共享和授权访问。

#pragma once

#include "core/ShareService.h"
#include "models/PeerInfo.h"
#include "models/SharedFolder.h"

#include <QHash>
#include <QList>
#include <QWidget>

class QLabel;
class QListWidget;
class QTableWidget;

namespace FengSui {

class BeaconService;
class ShareService;

class SharePage : public QWidget {
    Q_OBJECT

public:
    explicit SharePage(QWidget* parent = nullptr);

    void setShareService(ShareService* service);
    void setBeaconService(BeaconService* service);

private:
    void renderMyShares();
    void renderShareSources();
    void renderRemoteShares(const QString& peerId,
                            const QList<RemoteSharedFolder>& shares);
    void renderRemoteItems(const QString& peerId,
                           const QString& shareId,
                           const QString& path,
                           const QList<RemoteShareItem>& items);
    QWidget* createShareRow(const SharedFolder& folder);
    void showServiceMessage(const QString& message);
    void showBrowseMessage(const QString& message);
    void requestSelectedSourceShares();
    void requestSelectedShareItems();
    void openSelectedRemoteItem(int row);
    PeerInfo selectedPeer() const;

    BeaconService* m_beaconService = nullptr;
    ShareService* m_shareService = nullptr;
    QList<SharedFolder> m_sharedFolders;
    QHash<QString, PeerInfo> m_sharePeers;
    QHash<QString, QList<RemoteSharedFolder>> m_remoteSharesByPeer;
    QList<RemoteSharedFolder> m_currentRemoteShares;
    QList<RemoteShareItem> m_currentRemoteItems;
    QString m_currentPeerId;
    QString m_currentShareId;
    QString m_currentPath = QStringLiteral("/");
    QListWidget* m_sourceList = nullptr;
    QListWidget* m_remoteShareList = nullptr;
    QTableWidget* m_fileTable = nullptr;
    QLabel* m_breadcrumbLabel = nullptr;
    QLabel* m_browseHint = nullptr;
    QListWidget* m_myShareList = nullptr;
    QLabel* m_serviceHint = nullptr;
};

} // namespace FengSui
