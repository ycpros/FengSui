// BeaconService.cpp
// 局域网发现服务实现：维护本机广播和对端在线状态。

#include "core/BeaconService.h"

#include "app/AppSettings.h"
#include "core/ShareService.h"
#include "models/NetworkPolicy.h"
#include "network/UdpDiscovery.h"
#include "platform/InterfaceEnumerator.h"
#include "platform/PlatformUtils.h"

#include <QAbstractSocket>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QTimer>

namespace FengSui {

namespace {

// 任务 006 验收要求每 5 秒发送一次 presence.hello。
constexpr int kHelloIntervalMs = 5000;

// 对端超过 15 秒未刷新即视为离线，对应 06_protocol.md 的超时规则。
constexpr int kPeerTimeoutMs = 15000;

// 超时检查比心跳更频繁，保证离线事件接近 15 秒边界发出。
constexpr int kTimeoutCheckIntervalMs = 1000;

// hello 报文缺少端口时使用协议默认 TCP 监听端口。
constexpr quint16 kDefaultTcpPort = 8787;

bool isValidPort(int port)
{
    return port > 0 && port <= 65535;
}

QList<NetworkInterfaceInfo> orderedInterfaces()
{
    QList<NetworkInterfaceInfo> ordered = InterfaceEnumerator::candidates();
    const QList<NetworkInterfaceInfo> all = InterfaceEnumerator::enumerate();
    for (const NetworkInterfaceInfo& iface : all) {
        bool exists = false;
        for (const NetworkInterfaceInfo& current : ordered) {
            if (current.id == iface.id && current.ip == iface.ip) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            ordered.append(iface);
        }
    }
    return ordered;
}

} // namespace

BeaconService::BeaconService(AppSettings* settings,
                             NetworkPolicy* networkPolicy,
                             QObject* parent)
    : QObject(parent)
    , m_settings(settings)
    , m_networkPolicy(networkPolicy)
    , m_discovery(new UdpDiscovery(this))
    , m_helloTimer(new QTimer(this))
    , m_timeoutTimer(new QTimer(this))
{
    // hello 定时器发送完整设备摘要，确保新加入设备即使错过启动广播也能发现本机。
    m_helloTimer->setInterval(kHelloIntervalMs);
    connect(m_helloTimer, &QTimer::timeout, this, &BeaconService::sendHello);

    // 超时定时器独立于 hello 广播，避免发送失败时阻塞离线判定。
    m_timeoutTimer->setInterval(kTimeoutCheckIntervalMs);
    connect(m_timeoutTimer, &QTimer::timeout, this, &BeaconService::checkPeerTimeouts);

    // UDP 层只交付 JSON，业务字段校验统一在 BeaconService 完成。
    connect(m_discovery,
            &UdpDiscovery::datagramReceived,
            this,
            [this](const QJsonObject& message,
                   const QHostAddress& senderAddress,
                   quint16) {
                handleDatagram(message, senderAddress);
            });

    connect(m_discovery, &UdpDiscovery::errorOccurred, this, [](const QString& errorMessage) {
        qWarning() << "UDP discovery error:" << errorMessage;
    });
}

BeaconService::~BeaconService()
{
    stop();
}

void BeaconService::setNetworkPolicy(NetworkPolicy* networkPolicy)
{
    m_networkPolicy = networkPolicy;
}

void BeaconService::setShareService(ShareService* shareService)
{
    if (m_shareService == shareService) {
        return;
    }
    if (m_shareService) {
        disconnect(m_shareService, nullptr, this, nullptr);
    }

    m_shareService = shareService;
    if (m_shareService) {
        connect(m_shareService,
                &ShareService::shareAvailabilityChanged,
                this,
                [this](bool) {
                    if (m_running) {
                        sendHello();
                    }
                });
    }
}

bool BeaconService::start(QString& errorOut)
{
    if (m_running) {
        return true;
    }

    if (!m_settings) {
        errorOut = QStringLiteral("AppSettings is not available");
        return false;
    }

    if (!m_settings->discoveryEnabled()) {
        qInfo() << "UDP discovery is disabled by settings";
        return true;
    }

    // peer_id 必须稳定持久，不能在发现服务内临时生成。
    m_localPeerId = m_settings->peerId().trimmed();
    if (m_localPeerId.isEmpty()) {
        errorOut = QStringLiteral("Local peer_id is empty");
        return false;
    }

    const QList<DiscoveryEndpoint> endpoints = buildDiscoveryEndpoints();
    if (endpoints.isEmpty()) {
        errorOut = QStringLiteral("No authorized discovery interfaces");
        return false;
    }

    if (!m_discovery->start(endpoints, errorOut)) {
        return false;
    }

    m_running = true;

    // 启动后立即发送 hello，让同网段设备不必等待下一次心跳周期。
    sendHello();

    m_helloTimer->start();
    m_timeoutTimer->start();

    qInfo() << "Beacon service started for peer" << m_localPeerId;
    return true;
}

void BeaconService::stop()
{
    if (!m_running) {
        return;
    }

    m_helloTimer->stop();
    m_timeoutTimer->stop();

    QString sendError;
    // goodbye 是尽力广播，失败不阻止本地资源释放。
    if (!m_localPeerId.isEmpty()
        && !m_discovery->sendGoodbye(m_localPeerId, sendError)) {
        qWarning() << "Failed to send presence.goodbye:" << sendError;
    }

    m_discovery->stop();
    m_running = false;

    const QList<QString> peerIds = m_peers.keys();
    // 停止发现后本地不再维护在线状态，外部 UI 后续可直接显示离线。
    for (const QString& peerId : peerIds) {
        markPeerOffline(peerId, QStringLiteral("beacon stopped"));
    }

    qInfo() << "Beacon service stopped";
}

bool BeaconService::isRunning() const
{
    return m_running;
}

QList<PeerInfo> BeaconService::peers() const
{
    return m_peers.values();
}

QJsonObject BeaconService::buildHelloPayload(const DiscoveryEndpoint& endpoint) const
{
    // hello 承载完整设备摘要，heartbeat 只承载 peer_id。
    QJsonObject payload;
    payload.insert(QStringLiteral("peer_id"), m_localPeerId);
    payload.insert(QStringLiteral("display_name"), m_settings->displayName());
    payload.insert(QStringLiteral("device_name"), hostName());
    payload.insert(QStringLiteral("ip"), endpoint.ip.toString());
    payload.insert(QStringLiteral("tcp_port"), static_cast<int>(endpoint.tcpPort));
    payload.insert(QStringLiteral("os"), platformOs());
    payload.insert(QStringLiteral("share_enabled"),
                   m_shareService ? m_shareService->hasActiveShares() : false);
    payload.insert(QStringLiteral("version"), QCoreApplication::applicationVersion());

    QJsonObject address;
    address.insert(QStringLiteral("ip"), endpoint.ip.toString());
    address.insert(QStringLiteral("port"), static_cast<int>(endpoint.tcpPort));
    address.insert(QStringLiteral("interface"), endpoint.interfaceName);
    address.insert(QStringLiteral("scope"),
                   m_networkPolicy
                       ? NetworkPolicy::modeToString(m_networkPolicy->mode())
                       : QStringLiteral("secure_lan"));
    QJsonArray addresses;
    addresses.append(address);
    payload.insert(QStringLiteral("addresses"), addresses);

    return payload;
}

QList<DiscoveryEndpoint> BeaconService::buildDiscoveryEndpoints() const
{
    QList<DiscoveryEndpoint> endpoints;
    if (!m_networkPolicy || !m_settings) {
        return endpoints;
    }

    const QList<NetworkInterfaceInfo> interfaces = orderedInterfaces();
    QList<InterfaceAddress> addresses;
    for (const NetworkInterfaceInfo& iface : interfaces) {
        InterfaceAddress address;
        address.interfaceId = iface.id;
        address.interfaceName = iface.name;
        address.ip = iface.ip;
        addresses.append(address);
    }

    const QList<BindEndpoint> bindEndpoints =
        m_networkPolicy->getBindEndpoints(m_settings->listenPort(), addresses);
    for (const BindEndpoint& bind : bindEndpoints) {
        for (const NetworkInterfaceInfo& iface : interfaces) {
            if (iface.id == bind.interfaceId && iface.ip == bind.ip) {
                DiscoveryEndpoint endpoint;
                endpoint.interfaceId = iface.id;
                endpoint.interfaceName = iface.name;
                endpoint.ip = iface.ip;
                endpoint.prefixLength = iface.prefixLength;
                endpoint.tcpPort = bind.port;
                endpoints.append(endpoint);
                break;
            }
        }
    }
    return endpoints;
}

void BeaconService::handleDatagram(const QJsonObject& message,
                                   const QHostAddress& senderAddress)
{
    if (!m_networkPolicy || !m_networkPolicy->isAddressAllowed(senderAddress)) {
        qWarning() << "Ignoring presence datagram from unauthorized source:"
                   << senderAddress.toString();
        return;
    }

    const QString type = message.value(QStringLiteral("type")).toString();
    if (type.isEmpty()) {
        qWarning() << "Invalid presence datagram: missing type";
        return;
    }

    // 按协议 type 分发，未知类型不影响发现服务继续运行。
    if (type == QStringLiteral("presence.hello")) {
        handleHello(message, senderAddress);
    } else if (type == QStringLiteral("presence.heartbeat")) {
        handleHeartbeat(message);
    } else if (type == QStringLiteral("presence.goodbye")) {
        handleGoodbye(message);
    } else {
        qWarning() << "Unknown presence datagram type:" << type;
    }
}

void BeaconService::handleHello(const QJsonObject& message,
                                const QHostAddress& senderAddress)
{
    const QString peerId = message.value(QStringLiteral("peer_id")).toString().trimmed();
    if (peerId.isEmpty()) {
        qWarning() << "Invalid presence.hello: missing peer_id";
        return;
    }
    if (peerId == m_localPeerId) {
        return;
    }

    // hello 是唯一包含昵称、设备名、端口等完整信息的发现报文。
    PeerInfo peer;
    peer.peerId = peerId;
    peer.displayName = message.value(QStringLiteral("display_name")).toString(peerId);
    peer.deviceName = message.value(QStringLiteral("device_name")).toString(peerId);

    quint16 port = 0;
    if (!selectAnnouncedEndpoint(message, senderAddress, peer.ip, port)) {
        qWarning() << "Invalid presence.hello: no allowed announced endpoint"
                   << peerId << senderAddress.toString();
        return;
    }
    peer.port = port;
    peer.os = message.value(QStringLiteral("os")).toString();
    peer.online = true;
    peer.lastSeenAt = QDateTime::currentDateTimeUtc();
    peer.shareEnabled = message.value(QStringLiteral("share_enabled")).toBool(false);
    peer.version = message.value(QStringLiteral("version")).toString();

    const bool wasKnown = m_peers.contains(peerId);
    const PeerInfo oldPeer = m_peers.value(peerId);
    const bool wasOnline = wasKnown && oldPeer.online;
    const bool infoChanged = !wasKnown
                             || oldPeer.displayName != peer.displayName
                             || oldPeer.deviceName != peer.deviceName
                             || oldPeer.ip != peer.ip
                             || oldPeer.port != peer.port
                             || oldPeer.os != peer.os
                             || oldPeer.shareEnabled != peer.shareEnabled
                             || oldPeer.version != peer.version;

    // 先更新缓存再发信号，保证槽函数查询 peers() 时能看到最新快照。
    m_peers.insert(peerId, peer);

    if (!wasOnline) {
        qInfo() << "Peer online:" << peer.displayName << peer.ip << peer.port;
    }
    if (!wasOnline || infoChanged) {
        emit peerOnline(peer);
    }
}

bool BeaconService::selectAnnouncedEndpoint(const QJsonObject& message,
                                            const QHostAddress& senderAddress,
                                            QString& ipOut,
                                            quint16& portOut) const
{
    if (!m_networkPolicy) {
        return false;
    }

    const QJsonArray addresses =
        message.value(QStringLiteral("addresses")).toArray();
    for (const QJsonValue& value : addresses) {
        const QJsonObject object = value.toObject();
        const QString ip = object.value(QStringLiteral("ip")).toString().trimmed();
        const int port = object.value(QStringLiteral("port"))
                             .toInt(message.value(QStringLiteral("tcp_port"))
                                        .toInt(kDefaultTcpPort));
        const QHostAddress address(ip);
        if (address.protocol() != QAbstractSocket::IPv4Protocol) {
            continue;
        }
        if (!isValidPort(port)) {
            continue;
        }
        if (!m_networkPolicy->isAddressAllowed(address)) {
            continue;
        }

        ipOut = address.toString();
        portOut = static_cast<quint16>(port);
        return true;
    }
    if (!addresses.isEmpty()) {
        return false;
    }

    const QString announcedIp =
        message.value(QStringLiteral("ip")).toString().trimmed();
    const QHostAddress address =
        announcedIp.isEmpty() ? senderAddress : QHostAddress(announcedIp);
    const int port = message.value(QStringLiteral("tcp_port")).toInt(kDefaultTcpPort);
    if (address.protocol() != QAbstractSocket::IPv4Protocol || !isValidPort(port)) {
        return false;
    }
    if (!m_networkPolicy->isAddressAllowed(address)) {
        return false;
    }

    ipOut = address.toString();
    portOut = static_cast<quint16>(port);
    return true;
}

void BeaconService::handleHeartbeat(const QJsonObject& message)
{
    const QString peerId = message.value(QStringLiteral("peer_id")).toString().trimmed();
    if (peerId.isEmpty()) {
        qWarning() << "Invalid presence.heartbeat: missing peer_id";
        return;
    }
    if (peerId == m_localPeerId) {
        return;
    }

    auto it = m_peers.find(peerId);
    if (it == m_peers.end()) {
        // heartbeat 不包含设备摘要，未知设备必须等待下一次 hello 再加入列表。
        qDebug() << "Ignoring heartbeat from unknown peer:" << peerId;
        return;
    }

    PeerInfo& peer = it.value();
    // heartbeat 只刷新在线时间，不覆盖 hello 中的设备摘要。
    peer.lastSeenAt = QDateTime::currentDateTimeUtc();
    if (!peer.online) {
        peer.online = true;
        qInfo() << "Peer online by heartbeat:" << peer.displayName << peer.ip << peer.port;
        emit peerOnline(peer);
    }
}

void BeaconService::handleGoodbye(const QJsonObject& message)
{
    const QString peerId = message.value(QStringLiteral("peer_id")).toString().trimmed();
    if (peerId.isEmpty()) {
        qWarning() << "Invalid presence.goodbye: missing peer_id";
        return;
    }
    if (peerId == m_localPeerId) {
        return;
    }

    markPeerOffline(peerId, QStringLiteral("goodbye"));
}

void BeaconService::sendHello()
{
    if (!m_running || m_localPeerId.isEmpty()) {
        return;
    }

    QString error;
    for (const DiscoveryEndpoint& endpoint : m_discovery->discoveryEndpoints()) {
        QString endpointError;
        if (!m_discovery->sendHello(buildHelloPayload(endpoint),
                                    endpoint,
                                    endpointError)) {
            error = endpointError;
            qWarning() << "Failed to send presence.hello on"
                       << endpoint.interfaceName
                       << endpoint.ip.toString()
                       << endpointError;
        }
    }
}

void BeaconService::sendHeartbeat()
{
    if (!m_running || m_localPeerId.isEmpty()) {
        return;
    }

    QString error;
    // 保留 heartbeat 发送能力，供后续需要轻量心跳时复用。
    if (!m_discovery->sendHeartbeat(m_localPeerId, error)) {
        qWarning() << "Failed to send presence.heartbeat:" << error;
    }
}

void BeaconService::checkPeerTimeouts()
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    QList<QString> timedOutPeerIds;

    // 遍历时先收集 peer_id，避免在 const 迭代过程中修改哈希表。
    for (auto it = m_peers.cbegin(); it != m_peers.cend(); ++it) {
        const PeerInfo& peer = it.value();
        if (peer.online
            && (!peer.lastSeenAt.isValid()
                || peer.lastSeenAt.msecsTo(now) > kPeerTimeoutMs)) {
            timedOutPeerIds.append(peer.peerId);
        }
    }

    for (const QString& peerId : timedOutPeerIds) {
        markPeerOffline(peerId, QStringLiteral("heartbeat timeout"));
    }
}

void BeaconService::markPeerOffline(const QString& peerId, const QString& reason)
{
    auto it = m_peers.find(peerId);
    if (it == m_peers.end() || !it.value().online) {
        return;
    }

    PeerInfo& peer = it.value();
    // 保留设备记录，仅切换在线状态，便于后续 UI 显示最近发现过的设备。
    peer.online = false;
    qInfo() << "Peer offline:" << peerId << reason;
    emit peerOffline(peerId);
}

} // namespace FengSui
