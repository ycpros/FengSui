// ContactsViewModel.h
// 联系人页 ViewModel：聚合在线设备列表模型，连接 BeaconService 上线/下线信号，
// 处理手动添加设备（TCP 探测 + 持久化）。QML 只依赖此 VM 与 PeerListModel。

#pragma once

#include "models/PeerInfo.h"
#include "ui/viewmodels/PeerListModel.h"

#include <QObject>
#include <QString>

namespace FengSui {

class BeaconService;
class NetworkPolicy;
class ManualPeerRepository;

class ContactsViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(PeerListModel* peers READ peers CONSTANT)
    Q_PROPERTY(int peerCount READ peerCount NOTIFY peerCountChanged)

public:
    explicit ContactsViewModel(QObject* parent = nullptr);

    // 绑定服务（向导完成、服务启动后调用）。可重复调用以重连。
    void bind(BeaconService* beacon,
              NetworkPolicy* policy,
              ManualPeerRepository* manualRepo);

    PeerListModel* peers() const { return m_model; }
    int peerCount() const;

    // 手动添加设备：校验 → TCP 探测 → 成功则加入列表并持久化。
    // 返回空串表示成功；否则返回可读错误信息（QML 直接展示）。
    Q_INVOKABLE QString addManualPeer(const QString& ip, int port);

    // 双击某行打开会话：转发 peerId 给上层（AppController 切到聊天页）。
    Q_INVOKABLE void activatePeer(int row);

signals:
    void peerCountChanged();
    // 请求打开与某设备的会话（由 AppController 接收）
    void requestOpenConversation(const QString& peerId);

private slots:
    void onPeerOnline(PeerInfo peer);
    void onPeerOffline(QString peerId);

private:
    PeerListModel*        m_model = nullptr;
    BeaconService*        m_beacon = nullptr;
    NetworkPolicy*        m_policy = nullptr;
    ManualPeerRepository* m_manualRepo = nullptr;
};

} // namespace FengSui
