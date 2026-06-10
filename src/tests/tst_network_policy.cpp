// tst_network_policy.cpp
// NetworkPolicy 与 InterfaceEnumerator 的单元测试。
// 覆盖 CIDR 判定、端点生成、网卡枚举过滤。
#include "models/NetworkPolicy.h"
#include "platform/InterfaceEnumerator.h"

#include <QHash>
#include <QHostAddress>
#include <QPair>
#include <QtTest/QtTest>

namespace {

class NetworkPolicyTest : public QObject {
    Q_OBJECT

private slots:
    void defaultState();
    void isAddressAllowedSingleCidr();
    void isAddressAllowedMultipleCidrs();
    void isAddressAllowedBoundary();
    void isCidrAllowed();
    void isValidCidr_Valid();
    void isValidCidr_Invalid();
    void modeStringRoundTrip();
    void getBindEndpointsSecureLan();
    void getBindEndpointsMultiLan();
    void getBindEndpointsCompatTest();
    void interfaceEnumeratorFiltersLoopback();
};

void NetworkPolicyTest::defaultState()
{
    FengSui::NetworkPolicy policy;
    QCOMPARE(policy.mode(), FengSui::NetworkMode::secure_lan);
    QVERIFY(policy.selectedInterfaces().isEmpty());
    QVERIFY(policy.allowedCidrs().isEmpty());
    QVERIFY(policy.getAllowedSubnets().isEmpty());
    // 空 CIDR 列表时任何地址都不被允许
    QVERIFY(!policy.isAddressAllowed(QHostAddress("192.168.1.1")));
    QVERIFY(!policy.isCidrAllowed(QStringLiteral("192.168.1.0/24")));
}

void NetworkPolicyTest::isAddressAllowedSingleCidr()
{
    FengSui::NetworkPolicy policy;
    policy.setAllowedCidrs({"192.168.10.0/24"});

    // 网段内
    QVERIFY(policy.isAddressAllowed(QHostAddress("192.168.10.1")));
    QVERIFY(policy.isAddressAllowed(QHostAddress("192.168.10.25")));
    QVERIFY(policy.isAddressAllowed(QHostAddress("192.168.10.254")));

    // 网段外
    QVERIFY(!policy.isAddressAllowed(QHostAddress("192.168.11.1")));
    QVERIFY(!policy.isAddressAllowed(QHostAddress("10.0.0.1")));
    QVERIFY(!policy.isAddressAllowed(QHostAddress("172.16.0.1")));
}

void NetworkPolicyTest::isAddressAllowedMultipleCidrs()
{
    FengSui::NetworkPolicy policy;
    policy.setAllowedCidrs({"192.168.10.0/24", "10.20.0.0/16"});

    // 命中第一个 CIDR
    QVERIFY(policy.isAddressAllowed(QHostAddress("192.168.10.50")));
    // 命中第二个 CIDR
    QVERIFY(policy.isAddressAllowed(QHostAddress("10.20.5.8")));
    // 都不命中
    QVERIFY(!policy.isAddressAllowed(QHostAddress("172.16.0.1")));
}

void NetworkPolicyTest::isAddressAllowedBoundary()
{
    FengSui::NetworkPolicy policy;
    policy.setAllowedCidrs({"192.168.10.0/24"});

    // 网络地址（.0）和广播地址（.255）也在网段内
    QVERIFY(policy.isAddressAllowed(QHostAddress("192.168.10.0")));
    QVERIFY(policy.isAddressAllowed(QHostAddress("192.168.10.255")));
    // 首尾可用 IP
    QVERIFY(policy.isAddressAllowed(QHostAddress("192.168.10.1")));
    QVERIFY(policy.isAddressAllowed(QHostAddress("192.168.10.254")));
    // 刚好在网段外
    QVERIFY(!policy.isAddressAllowed(QHostAddress("192.168.9.255")));
    QVERIFY(!policy.isAddressAllowed(QHostAddress("192.168.11.0")));
}

void NetworkPolicyTest::isCidrAllowed()
{
    FengSui::NetworkPolicy policy;
    policy.setAllowedCidrs({"192.168.10.0/24", "10.20.0.0/16"});

    QVERIFY(policy.isCidrAllowed(QStringLiteral("192.168.10.0/25")));
    QVERIFY(policy.isCidrAllowed(QStringLiteral("10.20.5.0/24")));
    QVERIFY(!policy.isCidrAllowed(QStringLiteral("192.168.0.0/16")));
    QVERIFY(!policy.isCidrAllowed(QStringLiteral("10.21.0.0/16")));
    QVERIFY(!policy.isCidrAllowed(QStringLiteral("invalid")));
}

void NetworkPolicyTest::isValidCidr_Valid()
{
    QVERIFY(FengSui::NetworkPolicy::isValidCidr("192.168.1.0/24"));
    QVERIFY(FengSui::NetworkPolicy::isValidCidr("10.0.0.0/8"));
    QVERIFY(FengSui::NetworkPolicy::isValidCidr("172.16.0.0/12"));
    QVERIFY(FengSui::NetworkPolicy::isValidCidr("0.0.0.0/0"));
    QVERIFY(FengSui::NetworkPolicy::isValidCidr("192.168.1.1/32"));
}

void NetworkPolicyTest::isValidCidr_Invalid()
{
    QVERIFY(!FengSui::NetworkPolicy::isValidCidr("invalid"));
    QVERIFY(!FengSui::NetworkPolicy::isValidCidr("192.168.1.0/33"));
    QVERIFY(!FengSui::NetworkPolicy::isValidCidr("192.168.1.0/-1"));
    QVERIFY(!FengSui::NetworkPolicy::isValidCidr("/24"));
    QVERIFY(!FengSui::NetworkPolicy::isValidCidr("not_an_ip/24"));
    QVERIFY(!FengSui::NetworkPolicy::isValidCidr(""));
}

void NetworkPolicyTest::modeStringRoundTrip()
{
    QCOMPARE(FengSui::NetworkPolicy::modeFromString(QStringLiteral("secure_lan")),
             FengSui::NetworkMode::secure_lan);
    QCOMPARE(FengSui::NetworkPolicy::modeFromString(QStringLiteral("multi_lan")),
             FengSui::NetworkMode::multi_lan);
    QCOMPARE(FengSui::NetworkPolicy::modeFromString(QStringLiteral("compat_test")),
             FengSui::NetworkMode::compat_test);
    QCOMPARE(FengSui::NetworkPolicy::modeFromString(QStringLiteral("unknown")),
             FengSui::NetworkMode::secure_lan);
    QCOMPARE(FengSui::NetworkPolicy::modeToString(FengSui::NetworkMode::multi_lan),
             QStringLiteral("multi_lan"));
}

void NetworkPolicyTest::getBindEndpointsSecureLan()
{
    FengSui::NetworkPolicy policy;
    policy.setMode(FengSui::NetworkMode::secure_lan);
    policy.setSelectedInterfaces({"eth0", "eth1"});
    policy.setAllowedCidrs({"192.168.10.0/24", "10.20.0.0/16"});

    QHash<QString, QPair<QString, QHostAddress>> ifaceIps;
    ifaceIps["eth0"] = {"Ethernet", QHostAddress("192.168.10.25")};
    ifaceIps["eth1"] = {"Ethernet 2", QHostAddress("10.20.5.8")};

    const auto endpoints = policy.getBindEndpoints(8787, ifaceIps);

    // secure_lan 仅返回第一个匹配的端点；QHash 兼容路径按 key 排序
    QCOMPARE(endpoints.size(), 1);
    QCOMPARE(endpoints[0].interfaceId, QStringLiteral("eth0"));
    QCOMPARE(endpoints[0].interfaceName, QStringLiteral("Ethernet"));
    QCOMPARE(endpoints[0].port, static_cast<quint16>(8787));
}

void NetworkPolicyTest::getBindEndpointsMultiLan()
{
    FengSui::NetworkPolicy policy;
    policy.setMode(FengSui::NetworkMode::multi_lan);
    policy.setSelectedInterfaces({"eth0", "eth1"});
    policy.setAllowedCidrs({"192.168.10.0/24", "10.20.0.0/16"});

    QHash<QString, QPair<QString, QHostAddress>> ifaceIps;
    ifaceIps["eth0"] = {"Ethernet", QHostAddress("192.168.10.25")};
    ifaceIps["eth1"] = {"Ethernet 2", QHostAddress("10.20.5.8")};

    const auto endpoints = policy.getBindEndpoints(8787, ifaceIps);

    // multi_lan 返回所有匹配的端点
    QCOMPARE(endpoints.size(), 2);
}

void NetworkPolicyTest::getBindEndpointsCompatTest()
{
    FengSui::NetworkPolicy policy;
    policy.setMode(FengSui::NetworkMode::compat_test);
    // 无授权网卡

    QHash<QString, QPair<QString, QHostAddress>> ifaceIps;
    const auto endpoints = policy.getBindEndpoints(8787, ifaceIps);

    // compat_test 无授权网卡时返回空列表（调用方解释为 0.0.0.0）
    QVERIFY(endpoints.isEmpty());
}

void NetworkPolicyTest::interfaceEnumeratorFiltersLoopback()
{
    // candidates() 不应包含回环或链路本地
    const auto candidates = FengSui::InterfaceEnumerator::candidates();
    for (const auto &iface : candidates) {
        QVERIFY(iface.type != FengSui::InterfaceType::Loopback);
        QVERIFY(iface.type != FengSui::InterfaceType::LinkLocal);
        QVERIFY(iface.type != FengSui::InterfaceType::Virtual);
        QVERIFY(iface.type != FengSui::InterfaceType::Public);
        QCOMPARE(iface.type, FengSui::InterfaceType::Physical);
    }
}

} // namespace

QTEST_MAIN(NetworkPolicyTest)

#include "tst_network_policy.moc"
