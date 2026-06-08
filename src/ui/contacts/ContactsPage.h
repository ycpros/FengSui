// ContactsPage.h
// 联系人/在线设备列表页面。
// 展示 BeaconService 发现到的在线设备，支持手动输入 IP 添加设备。

#pragma once

#include <QHash>
#include <QString>
#include <QWidget>

#include "models/PeerInfo.h"

class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;

namespace FengSui {

class BeaconService;

// ContactsPage 负责展示局域网在线设备。
// 不直接访问 network 或 storage 层，所有设备状态来自 BeaconService 的信号。
// 支持通过"添加设备"按钮手动输入 IP:端口连接设备。
// 线程安全性：UI 控件仅在主线程访问。
class ContactsPage : public QWidget {
    Q_OBJECT

public:
    // 创建联系人页。
    // beaconService: 局域网发现服务实例，可为空；为空时页面仅显示空态。
    // parent: Qt 父对象，用于资源释放。
    // 线程安全性：仅在主线程构造。
    explicit ContactsPage(BeaconService* beaconService, QWidget* parent = nullptr);

public slots:
    // 将手动添加的设备加入在线列表。
    // peer: 已填充的 PeerInfo（通常来自 AddPeerDialog）。
    // 会设置 online=true 和 lastSeenAt 时间戳。
    void addManualPeerSlot(PeerInfo peer);

signals:
    // 用户双击在线设备时发出。
    // peer: 被激活的在线设备信息，调用方可据此进入后续聊天流程。
    void peerActivated(PeerInfo peer);

private slots:
    // 处理设备上线或信息更新事件。
    void onPeerOnline(PeerInfo peer);

    // 处理设备离线事件。
    void onPeerOffline(QString peerId);

    // 处理列表项双击事件。
    void onItemDoubleClicked(QListWidgetItem* item);

    // 点击"添加设备"按钮，弹出手动添加对话框。
    void onAddClicked();

private:
    // 创建联系人页控件和布局。
    void setupUi();

    // 连接 BeaconService 信号并加载当前在线快照。
    void connectBeaconService();

    // 新增或更新一个在线设备行。
    void upsertPeer(const PeerInfo& peer);

    // 移除指定设备行。
    void removePeer(const QString& peerId);

    // 根据在线设备数量切换空态和列表显示。
    void updateEmptyState();

    // 为在线设备创建列表行控件。
    QWidget* createPeerRow(const PeerInfo& peer);

    // 将系统标识转换为无资源依赖的文本徽标。
    QString osBadgeText(const QString& os) const;

    // 生成 IP:端口文本。
    QString endpointText(const PeerInfo& peer) const;

    // 检查指定 IP:端口是否已在列表中。
    bool hasPeerAtEndpoint(const QString& ip, quint16 port) const;

    BeaconService* m_beaconService = nullptr;
    QLabel* m_emptyLabel = nullptr;
    QListWidget* m_peerList = nullptr;
    QPushButton* m_addBtn = nullptr;
    QHash<QString, PeerInfo> m_onlinePeers;
};

} // namespace FengSui
