// ContactsViewModel.cpp
// 联系人页 ViewModel 实现。

#include "ui/viewmodels/ContactsViewModel.h"

#include "ui/viewmodels/PeerListModel.h"
#include "core/BeaconService.h"
#include "core/TcpProbe.h"
#include "models/NetworkPolicy.h"
#include "storage/ManualPeerRepository.h"

#include <QDateTime>
#include <QHostAddress>
#include <QNetworkInterface>

namespace FengSui {

namespace {
constexpr int kProbeTimeoutMs = 3000;

// 判断目标是否为本机地址（回环或本机任一 IPv4），复用自旧 AddPeerDialog 逻辑。
bool isSelfConnection(const QString& ip)
{
    const QHostAddress target(ip);
    if (target.isLoopback()) {
        return true;
    }
    const QList<QHostAddress> localAddrs = QNetworkInterface::allAddresses();
    for (const QHostAddress& local : localAddrs) {
        if (local.protocol() != QAbstractSocket::IPv4Protocol || local.isLoopback()) {
            continue;
        }
        if (local == target) {
            return true;
        }
    }
    return false;
}
} // namespace

ContactsViewModel::ContactsViewModel(QObject* parent)
    : QObject(parent)
    , m_model(new PeerListModel(this))
{
    connect(m_model, &PeerListModel::countChanged,
            this, &ContactsViewModel::peerCountChanged);
}

void ContactsViewModel::bind(BeaconService* beacon,
                             NetworkPolicy* policy,
                             ManualPeerRepository* manualRepo)
{
    // 断开旧连接（若重复 bind）
    if (m_beacon) {
        disconnect(m_beacon, nullptr, this, nullptr);
    }

    m_beacon = beacon;
    m_policy = policy;
    m_manualRepo = manualRepo;

    if (m_beacon) {
        connect(m_beacon, &BeaconService::peerOnline,
                this, &ContactsViewModel::onPeerOnline);
        connect(m_beacon, &BeaconService::peerOffline,
                this, &ContactsViewModel::onPeerOffline);
        // 加载当前在线快照
        m_model->resetFrom(m_beacon->peers());
    }
}

int ContactsViewModel::peerCount() const
{
    return m_model ? m_model->rowCountProperty() : 0;
}

QString ContactsViewModel::addManualPeer(const QString& ipRaw, int portInt)
{
    const QString ip = ipRaw.trimmed();
    if (ip.isEmpty()) {
        return QStringLiteral("请输入 IP 地址");
    }

    const QHostAddress addr(ip);
    if (addr.isNull() || addr.protocol() != QAbstractSocket::IPv4Protocol) {
        return QStringLiteral("IP 地址格式无效");
    }

    if (m_policy && !m_policy->isAddressAllowed(addr)) {
        return QStringLiteral("目标 IP 不在允许网段内");
    }

    if (portInt < 1 || portInt > 65535) {
        return QStringLiteral("端口号范围 1-65535");
    }
    const quint16 port = static_cast<quint16>(portInt);

    if (isSelfConnection(ip)) {
        return QStringLiteral("不能连接本机地址");
    }

    QString probeError;
    if (!TcpProbe::probe(ip, port, kProbeTimeoutMs, probeError)) {
        return probeError;
    }

    // 探测成功：构造手动设备并加入列表
    PeerInfo peer;
    peer.peerId = QStringLiteral("manual:%1:%2").arg(ip).arg(port);
    peer.displayName = QStringLiteral("%1:%2").arg(ip).arg(port);
    peer.deviceName = QStringLiteral("手动添加");
    peer.ip = ip;
    peer.port = port;
    peer.online = true;
    peer.lastSeenAt = QDateTime::currentDateTimeUtc();
    m_model->upsert(peer);

    // 持久化手动设备
    if (m_manualRepo) {
        ManualPeer mp;
        mp.name = peer.displayName;
        mp.host = ip;
        mp.port = port;
        mp.lastSuccessAt = QDateTime::currentDateTimeUtc();
        m_manualRepo->upsertManualPeer(mp);
    }

    return QString();   // 空串表示成功
}

void ContactsViewModel::activatePeer(int row)
{
    const PeerInfo peer = m_model->at(row);
    if (!peer.peerId.isEmpty()) {
        emit requestOpenConversation(peer.peerId);
    }
}

void ContactsViewModel::onPeerOnline(PeerInfo peer)
{
    m_model->upsert(peer);
}

void ContactsViewModel::onPeerOffline(QString peerId)
{
    m_model->remove(peerId);
}

} // namespace FengSui
