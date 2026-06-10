// BeaconService.h
// 局域网设备自动发现服务，基于 UDP 组播实现节点上线/下线/心跳检测。
// 不依赖 Avahi / Bonjour 等系统 mDNS 守护进程。

#pragma once

#include "models/PeerInfo.h"
#include "network/UdpDiscovery.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

class QHostAddress;
class QJsonObject;
class QTimer;

namespace FengSui {

class AppSettings;
class NetworkPolicy;

// BeaconService 负责 UDP 发现业务状态：本机身份、对端在线表和心跳超时。
// 不直接操作界面或数据库，状态变化通过 Qt 信号通知外部。
// 线程安全性：内部使用 QTimer，回调在对象所属线程执行；当前设计为主线程使用。
class BeaconService : public QObject {
    Q_OBJECT

public:
    // 创建局域网发现服务。
    // settings: 应用设置实例，提供 peer_id、昵称、监听端口和发现开关，生命周期必须长于本对象。
    // parent: Qt 父对象，用于资源释放。
    // 线程安全性：仅在主线程或同一 QObject 所属线程构造。
    explicit BeaconService(AppSettings* settings,
                           NetworkPolicy* networkPolicy,
                           QObject* parent = nullptr);

    void setNetworkPolicy(NetworkPolicy* networkPolicy);

    // 测试辅助：不启动 UDP socket 时设置本机 peer_id。
    void setLocalPeerIdForTest(const QString& peerId) { m_localPeerId = peerId; }

    // 销毁局域网发现服务。
    // 析构时会调用 stop()，尽力广播离线消息并释放 UDP socket。
    // 线程安全性：仅在对象所属线程销毁。
    ~BeaconService() override;

    // 启动发现服务。
    // errorOut: 启动失败时写入可读错误信息。
    // 返回值：启动成功返回 true；发现被配置关闭时不启动 socket 但返回 true。
    // 线程安全性：仅在对象所属线程调用。
    bool start(QString& errorOut);

    // 停止发现服务，尽力广播 presence.goodbye。
    // 停止后会把当前在线设备标记为离线并发出 peerOffline()。
    // 线程安全性：仅在对象所属线程调用。
    void stop();

    // 返回发现服务是否正在运行。
    // 返回值：UDP socket 已启动并且 hello 定时器处于运行状态时为 true。
    // 线程安全性：仅在对象所属线程调用。
    bool isRunning() const;

    // 返回已发现设备快照，包含已超时标记离线的设备。
    // 返回值：PeerInfo 列表副本，调用方修改不会影响内部状态。
    // 线程安全性：仅在对象所属线程调用。
    QList<PeerInfo> peers() const;

signals:
    // 对端上线或信息更新。
    // peer: 最新的在线设备信息，online 字段为 true。
    void peerOnline(PeerInfo peer);

    // 对端主动离线或心跳超时。
    // peerId: 离线设备的唯一标识。
    void peerOffline(QString peerId);

private:
    // 构造本机 presence.hello 负载。
    QJsonObject buildHelloPayload(const DiscoveryEndpoint& endpoint) const;

    QList<DiscoveryEndpoint> buildDiscoveryEndpoints() const;

    // 处理 UDP 层收到的 JSON 报文。
    void handleDatagram(const QJsonObject& message, const QHostAddress& senderAddress);

    // 处理 presence.hello。
    void handleHello(const QJsonObject& message, const QHostAddress& senderAddress);

    bool selectAnnouncedEndpoint(const QJsonObject& message,
                                 const QHostAddress& senderAddress,
                                 QString& ipOut,
                                 quint16& portOut) const;

    // 处理 presence.heartbeat。
    void handleHeartbeat(const QJsonObject& message);

    // 处理 presence.goodbye。
    void handleGoodbye(const QJsonObject& message);

    // 发送一次完整设备摘要广播。
    void sendHello();

    // 发送一次协议心跳。
    void sendHeartbeat();

    // 检查 15 秒未更新的在线设备。
    void checkPeerTimeouts();

    // 将指定设备标记为离线。
    void markPeerOffline(const QString& peerId, const QString& reason);

    AppSettings* m_settings = nullptr;
    NetworkPolicy* m_networkPolicy = nullptr;
    UdpDiscovery* m_discovery = nullptr;
    QTimer* m_helloTimer = nullptr;
    QTimer* m_timeoutTimer = nullptr;
    QHash<QString, PeerInfo> m_peers;
    QString m_localPeerId;
    bool m_running = false;
};

} // namespace FengSui
