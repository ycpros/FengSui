// tst_beacon_policy.cpp
// BeaconService 发现报文策略校验测试。

#include "core/BeaconService.h"
#include "core/ShareService.h"
#include "models/NetworkPolicy.h"
#include "network/UdpDiscovery.h"
#include "storage/Database.h"
#include "storage/ShareRepository.h"

#include <QHostAddress>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

namespace {

class BeaconPolicyTest : public QObject {
    Q_OBJECT

private slots:
    void allowedHelloCreatesPeer();
    void unauthorizedSourceIsIgnored();
    void unauthorizedAnnouncedAddressIsIgnored();
    void emptyPeerIdIsIgnored();
    void selfPeerIdIsIgnored();
    void shareEnabledFlagIsStored();
    void shareServiceCanBeInjected();

private:
    static QJsonObject hello(const QString& peerId,
                             const QString& ip,
                             int port = 8787);
    static void deliver(FengSui::BeaconService& service,
                        const QJsonObject& message,
                        const QString& sourceIp);
};

QJsonObject BeaconPolicyTest::hello(const QString& peerId,
                                    const QString& ip,
                                    int port)
{
    QJsonObject address;
    address.insert(QStringLiteral("ip"), ip);
    address.insert(QStringLiteral("port"), port);

    QJsonArray addresses;
    addresses.append(address);

    QJsonObject message;
    message.insert(QStringLiteral("type"), QStringLiteral("presence.hello"));
    message.insert(QStringLiteral("peer_id"), peerId);
    message.insert(QStringLiteral("display_name"), QStringLiteral("Remote"));
    message.insert(QStringLiteral("device_name"), QStringLiteral("Remote-PC"));
    message.insert(QStringLiteral("tcp_port"), port);
    message.insert(QStringLiteral("addresses"), addresses);
    return message;
}

void BeaconPolicyTest::deliver(FengSui::BeaconService& service,
                               const QJsonObject& message,
                               const QString& sourceIp)
{
    auto* discovery = service.findChild<FengSui::UdpDiscovery*>();
    QVERIFY(discovery);
    emit discovery->datagramReceived(message, QHostAddress(sourceIp), 8788);
}

void BeaconPolicyTest::allowedHelloCreatesPeer()
{
    FengSui::NetworkPolicy policy;
    policy.setAllowedCidrs({QStringLiteral("192.168.10.0/24")});

    FengSui::BeaconService service(nullptr, &policy);
    service.setLocalPeerIdForTest(QStringLiteral("peer_local"));
    deliver(service,
            hello(QStringLiteral("peer_remote"), QStringLiteral("192.168.10.22")),
            QStringLiteral("192.168.10.22"));

    const QList<FengSui::PeerInfo> peers = service.peers();
    QCOMPARE(peers.size(), 1);
    QCOMPARE(peers.first().ip, QStringLiteral("192.168.10.22"));
}

void BeaconPolicyTest::shareEnabledFlagIsStored()
{
    FengSui::NetworkPolicy policy;
    policy.setAllowedCidrs({QStringLiteral("192.168.10.0/24")});

    FengSui::BeaconService service(nullptr, &policy);
    service.setLocalPeerIdForTest(QStringLiteral("peer_local"));
    QJsonObject message =
        hello(QStringLiteral("peer_remote"), QStringLiteral("192.168.10.22"));
    message.insert(QStringLiteral("share_enabled"), true);
    deliver(service, message, QStringLiteral("192.168.10.22"));

    const QList<FengSui::PeerInfo> peers = service.peers();
    QCOMPARE(peers.size(), 1);
    QVERIFY(peers.first().shareEnabled);
}

void BeaconPolicyTest::shareServiceCanBeInjected()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    FengSui::Database database(tempDir.filePath(QStringLiteral("share.db")));
    QVERIFY(database.initialize());
    FengSui::ShareRepository repository(&database);
    FengSui::ShareService shareService;
    shareService.setShareRepository(&repository);

    FengSui::NetworkPolicy policy;
    FengSui::BeaconService service(nullptr, &policy);
    service.setShareService(&shareService);

    QSignalSpy availabilitySpy(
        &shareService,
        &FengSui::ShareService::shareAvailabilityChanged);
    QVERIFY(shareService.addSharedFolder(tempDir.path()).has_value());
    QCOMPARE(availabilitySpy.size(), 1);

    service.setShareService(nullptr);
}

void BeaconPolicyTest::unauthorizedSourceIsIgnored()
{
    FengSui::NetworkPolicy policy;
    policy.setAllowedCidrs({QStringLiteral("192.168.10.0/24")});

    FengSui::BeaconService service(nullptr, &policy);
    service.setLocalPeerIdForTest(QStringLiteral("peer_local"));
    deliver(service,
            hello(QStringLiteral("peer_remote"), QStringLiteral("192.168.10.22")),
            QStringLiteral("10.0.0.5"));

    QVERIFY(service.peers().isEmpty());
}

void BeaconPolicyTest::unauthorizedAnnouncedAddressIsIgnored()
{
    FengSui::NetworkPolicy policy;
    policy.setAllowedCidrs({QStringLiteral("192.168.10.0/24")});

    FengSui::BeaconService service(nullptr, &policy);
    service.setLocalPeerIdForTest(QStringLiteral("peer_local"));
    deliver(service,
            hello(QStringLiteral("peer_remote"), QStringLiteral("10.0.0.5")),
            QStringLiteral("192.168.10.22"));

    QVERIFY(service.peers().isEmpty());
}

void BeaconPolicyTest::emptyPeerIdIsIgnored()
{
    FengSui::NetworkPolicy policy;
    policy.setAllowedCidrs({QStringLiteral("192.168.10.0/24")});

    FengSui::BeaconService service(nullptr, &policy);
    service.setLocalPeerIdForTest(QStringLiteral("peer_local"));
    deliver(service,
            hello(QString(), QStringLiteral("192.168.10.22")),
            QStringLiteral("192.168.10.22"));

    QVERIFY(service.peers().isEmpty());
}

void BeaconPolicyTest::selfPeerIdIsIgnored()
{
    FengSui::NetworkPolicy policy;
    policy.setAllowedCidrs({QStringLiteral("192.168.10.0/24")});

    FengSui::BeaconService service(nullptr, &policy);
    service.setLocalPeerIdForTest(QStringLiteral("peer_local"));
    deliver(service,
            hello(QStringLiteral("peer_local"), QStringLiteral("192.168.10.22")),
            QStringLiteral("192.168.10.22"));

    QVERIFY(service.peers().isEmpty());
}

} // namespace

QTEST_MAIN(BeaconPolicyTest)

#include "tst_beacon_policy.moc"
