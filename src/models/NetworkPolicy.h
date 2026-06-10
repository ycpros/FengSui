// NetworkPolicy.h
// 长期网络策略纯逻辑模型：定义网络模式、绑定端点、CIDR 判定。
// 纯头文件，不依赖 network/ 或 storage/ 层。
// 对应 docs/14_network_model.md。
#pragma once

#include <QAbstractSocket>
#include <QHash>
#include <QHostAddress>
#include <QList>
#include <QPair>
#include <QSet>
#include <QString>
#include <QStringList>

#include <algorithm>

namespace FengSui {

// 网络模式枚举，默认 secure_lan（14_network_model.md §3）。
enum class NetworkMode {
    secure_lan,  // 安全内网模式：单授权网卡，最小暴露面
    multi_lan,   // 多网卡内网模式：多授权网卡，各自独立发现
    compat_test  // 测试/兼容模式：可监听 0.0.0.0，放宽限制
};

// 绑定端点描述，用于 TcpServer 多端点监听（14_network_model.md §5）。
struct BindEndpoint {
    QString      interfaceId;   // 网卡唯一标识（QNetworkInterface::name()）
    QString      interfaceName; // 网卡可读名称
    QHostAddress ip;            // 绑定地址
    quint16      port = 0;      // 绑定端口
};

// 可用于生成绑定端点的接口地址快照。调用方负责按优先级排序。
struct InterfaceAddress {
    QString      interfaceId;
    QString      interfaceName;
    QHostAddress ip;
};

// 网络策略纯逻辑模型。
// 负责授权接口管理、CIDR 网段判定和绑定端点生成。
class NetworkPolicy {
public:
    NetworkPolicy() = default;

    void setMode(NetworkMode mode) { m_mode = mode; }
    NetworkMode mode() const { return m_mode; }

    void setSelectedInterfaces(const QSet<QString> &ids) { m_selectedIfaces = ids; }
    QSet<QString> selectedInterfaces() const { return m_selectedIfaces; }

    void setAllowedCidrs(const QStringList &cidrs) { m_allowedCidrs = cidrs; }
    QStringList allowedCidrs() const { return m_allowedCidrs; }

    // 检查 IPv4 地址是否落在任一允许 CIDR 内。CIDR 列表为空时返回 false。
    bool isAddressAllowed(const QHostAddress &address) const;

    // 检查 CIDR 是否完全落在任一允许 CIDR 内。
    bool isCidrAllowed(const QString &cidr) const;

    // 验证 CIDR 字符串合法性（格式 + 前缀范围 0-32）
    static bool isValidCidr(const QString &cidr);

    static NetworkMode modeFromString(const QString &value);
    static QString modeToString(NetworkMode mode);

    // 从策略配置生成绑定端点列表。
    // ifaceIps: 调用方按优先级排序后的接口地址列表。
    // secure_lan 仅返回第一个匹配端点；multi_lan 返回所有；compat_test 无授权时返回空。
    QList<BindEndpoint> getBindEndpoints(
        quint16 port,
        const QList<InterfaceAddress> &ifaceIps) const;

    // 兼容旧测试/调用路径：QHash 输入会按 key 排序后再生成端点。
    QList<BindEndpoint> getBindEndpoints(
        quint16 port,
        const QHash<QString, QPair<QString, QHostAddress>> &ifaceIps) const;

    // 获取解析后的允许子网列表
    QList<QPair<QHostAddress, int>> getAllowedSubnets() const;

private:
    // 解析单个 CIDR 字符串为网络地址和前缀长度
    static bool parseCidr(const QString &cidr, QHostAddress &netOut, int &prefixOut);

    NetworkMode m_mode = NetworkMode::secure_lan;
    QSet<QString> m_selectedIfaces;
    QStringList m_allowedCidrs;
};

// ---- 内联实现 ----

inline bool NetworkPolicy::isAddressAllowed(const QHostAddress &address) const
{
    if (address.protocol() != QAbstractSocket::IPv4Protocol) return false;
    const quint32 ip = address.toIPv4Address();
    for (const QString &cidr : m_allowedCidrs) {
        QHostAddress network;
        int prefix = 0;
        if (!parseCidr(cidr, network, prefix)) continue;
        const quint32 mask  = (prefix == 0) ? 0 : (0xFFFFFFFFu << (32 - prefix));
        const quint32 netIp = network.toIPv4Address();
        if ((ip & mask) == (netIp & mask)) return true;
    }
    return false;
}

inline bool NetworkPolicy::isCidrAllowed(const QString &cidr) const
{
    QHostAddress network;
    int prefix = 0;
    if (!parseCidr(cidr, network, prefix)) return false;

    const quint32 mask = (prefix == 0) ? 0 : (0xFFFFFFFFu << (32 - prefix));
    const quint32 first = network.toIPv4Address() & mask;
    const quint32 last = first | ~mask;

    for (const QString &allowedCidr : m_allowedCidrs) {
        QHostAddress allowedNetwork;
        int allowedPrefix = 0;
        if (!parseCidr(allowedCidr, allowedNetwork, allowedPrefix)) continue;

        const quint32 allowedMask = (allowedPrefix == 0)
            ? 0 : (0xFFFFFFFFu << (32 - allowedPrefix));
        const quint32 allowedFirst = allowedNetwork.toIPv4Address() & allowedMask;
        if ((first & allowedMask) == allowedFirst
            && (last & allowedMask) == allowedFirst) {
            return true;
        }
    }

    return false;
}

inline bool NetworkPolicy::isValidCidr(const QString &cidr)
{
    QHostAddress ignored;
    int prefix = 0;
    return parseCidr(cidr, ignored, prefix);
}

inline NetworkMode NetworkPolicy::modeFromString(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("multi_lan")) {
        return NetworkMode::multi_lan;
    }
    if (normalized == QStringLiteral("compat_test")) {
        return NetworkMode::compat_test;
    }
    return NetworkMode::secure_lan;
}

inline QString NetworkPolicy::modeToString(NetworkMode mode)
{
    switch (mode) {
    case NetworkMode::multi_lan:
        return QStringLiteral("multi_lan");
    case NetworkMode::compat_test:
        return QStringLiteral("compat_test");
    case NetworkMode::secure_lan:
    default:
        return QStringLiteral("secure_lan");
    }
}

inline QList<BindEndpoint> NetworkPolicy::getBindEndpoints(
    quint16 port,
    const QList<InterfaceAddress> &ifaceIps) const
{
    QList<BindEndpoint> endpoints;
    for (const InterfaceAddress &iface : ifaceIps) {
        if (!m_selectedIfaces.contains(iface.interfaceId)) continue;
        if (!isAddressAllowed(iface.ip)) continue;
        BindEndpoint ep;
        ep.interfaceId   = iface.interfaceId;
        ep.interfaceName = iface.interfaceName;
        ep.ip            = iface.ip;
        ep.port          = port;
        endpoints.append(ep);
        if (m_mode == NetworkMode::secure_lan) break;
    }
    return endpoints;
}

inline QList<BindEndpoint> NetworkPolicy::getBindEndpoints(
    quint16 port,
    const QHash<QString, QPair<QString, QHostAddress>> &ifaceIps) const
{
    QList<QString> keys = ifaceIps.keys();
    std::sort(keys.begin(), keys.end());

    QList<InterfaceAddress> ordered;
    for (const QString &key : keys) {
        InterfaceAddress iface;
        iface.interfaceId = key;
        iface.interfaceName = ifaceIps.value(key).first;
        iface.ip = ifaceIps.value(key).second;
        ordered.append(iface);
    }
    return getBindEndpoints(port, ordered);
}

inline QList<QPair<QHostAddress, int>> NetworkPolicy::getAllowedSubnets() const
{
    QList<QPair<QHostAddress, int>> subnets;
    for (const QString &cidr : m_allowedCidrs) {
        QHostAddress network;
        int prefix = 0;
        if (parseCidr(cidr, network, prefix))
            subnets.append({network, prefix});
    }
    return subnets;
}

inline bool NetworkPolicy::parseCidr(const QString &cidr,
                                     QHostAddress &netOut,
                                     int &prefixOut)
{
    const int slashPos = cidr.indexOf(QLatin1Char('/'));
    if (slashPos < 0) return false;
    const QString ipPart     = cidr.left(slashPos).trimmed();
    const QString prefixPart = cidr.mid(slashPos + 1).trimmed();
    QHostAddress addr(ipPart);
    if (addr.isNull() || addr.protocol() != QAbstractSocket::IPv4Protocol) return false;
    bool ok = false;
    const int prefix = prefixPart.toInt(&ok);
    if (!ok || prefix < 0 || prefix > 32) return false;
    const quint32 mask    = (prefix == 0) ? 0 : (0xFFFFFFFFu << (32 - prefix));
    const quint32 network = addr.toIPv4Address() & mask;
    netOut    = QHostAddress(network);
    prefixOut = prefix;
    return true;
}

} // namespace FengSui
