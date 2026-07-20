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
    void resolveSelectedInterfaces();
    void deriveCidrsForSelectedInterfaces();
    void interfaceEnumeratorFiltersLoopback();
};

FengSui::NetworkInterfaceInfo interfaceInfo(const QString& id,
                                            const QString& ip,
                                            int prefixLength,
                                            bool physical = true)
{
    // 构造不依赖宿主机真实网络状态的合成网卡条目，使策略测试可稳定覆盖多地址、
    // 虚拟网卡和失效选择场景。
    FengSui::NetworkInterfaceInfo info;
    info.id = id;
    info.name = id;
    info.ip = QHostAddress(ip);
    info.prefixLength = prefixLength;
    info.isPhysical = physical;
    info.type = physical ? FengSui::InterfaceType::Physical
                         : FengSui::InterfaceType::Virtual;
    return info;
}

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

void NetworkPolicyTest::resolveSelectedInterfaces()
{
    const QList<FengSui::NetworkInterfaceInfo> candidates = {
        interfaceInfo(QStringLiteral("eth0"), QStringLiteral("192.168.1.20"), 24),
        interfaceInfo(QStringLiteral("vpn0"), QStringLiteral("10.0.0.2"), 24, false),
        interfaceInfo(QStringLiteral("eth1"), QStringLiteral("10.20.1.8"), 16),
    };

    // 有效选择保留原顺序；失效 ID、重复 ID 和非物理网卡 ID 都会被移除。
    QCOMPARE(FengSui::InterfaceEnumerator::resolveSelectedInterfaceIds(
                 {QStringLiteral("stale"), QStringLiteral("eth1"),
                  QStringLiteral("eth1"), QStringLiteral("vpn0")},
                 candidates),
             QStringList{QStringLiteral("eth1")});

    // 所有持久化选择失效时，回退到已排序候选列表中的首个物理网卡。
    QCOMPARE(FengSui::InterfaceEnumerator::resolveSelectedInterfaceIds(
                 {QStringLiteral("stale")}, candidates),
             QStringList{QStringLiteral("eth0")});

    // 系统没有任何候选网卡时保持空选择，等待下次应用启动重新检测。
    QVERIFY(FengSui::InterfaceEnumerator::resolveSelectedInterfaceIds(
                {QStringLiteral("stale")}, {}).isEmpty());

    // 只有虚拟网卡不构成物理候选，也不能成为自动回退目标。
    const QList<FengSui::NetworkInterfaceInfo> virtualOnly = {
        interfaceInfo(QStringLiteral("vpn0"), QStringLiteral("10.0.0.2"), 24, false),
    };
    QVERIFY(FengSui::InterfaceEnumerator::resolveSelectedInterfaceIds(
                {QStringLiteral("vpn0")}, virtualOnly).isEmpty());
}

void NetworkPolicyTest::deriveCidrsForSelectedInterfaces()
{
    const QList<FengSui::NetworkInterfaceInfo> interfaces = {
        interfaceInfo(QStringLiteral("eth0"), QStringLiteral("192.168.10.25"), 24),
        interfaceInfo(QStringLiteral("eth0"), QStringLiteral("192.168.10.90"), 24),
        interfaceInfo(QStringLiteral("eth0"), QStringLiteral("10.4.2.1"), 8),
        interfaceInfo(QStringLiteral("eth1"), QStringLiteral("172.16.5.8"), 16),
        interfaceInfo(QStringLiteral("eth2"), QStringLiteral("192.168.50.5"), 24),
        interfaceInfo(QStringLiteral("vpn0"), QStringLiteral("10.99.0.2"), 16, false),
    };

    // 同一网卡的多个 IPv4 地址全部参与推导；相同子网去重，未授权和虚拟网卡忽略。
    QCOMPARE(FengSui::InterfaceEnumerator::cidrsForSelectedInterfaces(
                 {QStringLiteral("eth0"), QStringLiteral("eth1"),
                  QStringLiteral("vpn0")},
                 interfaces),
             (QStringList{QStringLiteral("192.168.10.0/24"),
                          QStringLiteral("10.0.0.0/8"),
                          QStringLiteral("172.16.0.0/16")}));

    // 没有授权 ID 时不生成允许网段。
    QVERIFY(FengSui::InterfaceEnumerator::cidrsForSelectedInterfaces(
                {}, interfaces).isEmpty());

    // 即使虚拟网卡 ID 被异常传入，也不能生成允许网段。
    QVERIFY(FengSui::InterfaceEnumerator::cidrsForSelectedInterfaces(
                {QStringLiteral("vpn0")}, interfaces).isEmpty());
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
