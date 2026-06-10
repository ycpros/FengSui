// InterfaceEnumerator.cpp
// 本机网卡枚举与分类实现。
#include "platform/InterfaceEnumerator.h"

#include <QAbstractSocket>
#include <QList>
#include <QNetworkAddressEntry>
#include <QNetworkInterface>
#include <QString>

#include <algorithm>

namespace FengSui {

namespace {

// 虚拟网卡名称匹配模式列表（不区分大小写子串匹配）
const QStringList kVirtualPatterns = {
    "VMware", "vmnet", "VirtualBox", "vboxnet",
    "Hyper-V", "vEthernet",
    "WSL",
    "ZeroTier", "zt",
    "Tailscale",
    "Docker", "veth",
    "tun", "tap",
    "Bluetooth",
};

// 检查地址是否在 RFC1918 私有地址范围内
bool isRfc1918(const QHostAddress &addr)
{
    if (addr.protocol() != QAbstractSocket::IPv4Protocol) return false;
    const quint32 ip = addr.toIPv4Address();
    // 10.0.0.0/8
    if ((ip & 0xFF000000) == 0x0A000000) return true;
    // 172.16.0.0/12
    if ((ip & 0xFFF00000) == 0xAC100000) return true;
    // 192.168.0.0/16
    if ((ip & 0xFFFF0000) == 0xC0A80000) return true;
    return false;
}

// 检查地址是否在链路本地 169.254.0.0/16 范围内
bool isLinkLocal(const QHostAddress &addr)
{
    if (addr.protocol() != QAbstractSocket::IPv4Protocol) return false;
    const quint32 ip = addr.toIPv4Address();
    return (ip & 0xFFFF0000) == 0xA9FE0000;
}

// 检查网卡名称是否匹配虚拟网卡模式
bool isVirtualInterface(const QString &name)
{
    for (const QString &pattern : kVirtualPatterns) {
        if (name.contains(pattern, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

// 获取有默认网关的网卡名称集合。
// Qt 6 的 QNetworkAddressEntry 不暴露 gateway()，改用 addressEntries
// 中第一个有非空 netmask 的 IPv4 条目作为"活跃"网卡指标。
QStringList defaultGatewayInterfaces()
{
    QStringList result;
    const QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : ifaces) {
        const QList<QNetworkAddressEntry> entries = iface.addressEntries();
        for (const QNetworkAddressEntry &entry : entries) {
            if (!entry.ip().isNull()
                && entry.ip().protocol() == QAbstractSocket::IPv4Protocol
                && !entry.netmask().isNull()) {
                result.append(iface.name());
                break;
            }
        }
    }
    return result;
}

} // namespace

InterfaceType InterfaceEnumerator::classify(const QString &name,
                                             const QHostAddress &address,
                                             bool isUp,
                                             bool isRunning)
{
    if (address.isLoopback()) return InterfaceType::Loopback;
    if (isLinkLocal(address))  return InterfaceType::LinkLocal;
    if (isVirtualInterface(name)) return InterfaceType::Virtual;
    if (!isRfc1918(address))   return InterfaceType::Public;

    // 物理网卡：已连接、非回环、非链路本地、非虚拟、内网地址
    if (isUp && isRunning) return InterfaceType::Physical;
    return InterfaceType::Unknown;
}

QList<NetworkInterfaceInfo> InterfaceEnumerator::enumerate()
{
    QList<NetworkInterfaceInfo> result;
    const QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();

    for (const QNetworkInterface &iface : ifaces) {
        const QList<QNetworkAddressEntry> entries = iface.addressEntries();
        for (const QNetworkAddressEntry &entry : entries) {
            const QHostAddress &addr = entry.ip();
            if (addr.protocol() != QAbstractSocket::IPv4Protocol) continue;
            // enumerate 也排除回环，回环对发现和连接无意义
            if (addr.isLoopback()) continue;

            NetworkInterfaceInfo info;
            info.id           = iface.name();
            info.name         = iface.humanReadableName();
            info.ip           = addr;
            info.prefixLength = entry.prefixLength();
            if (info.prefixLength <= 0)
                info.prefixLength = 24;  // 默认 /24
            info.type = classify(iface.name(), addr,
                                 iface.flags().testFlag(QNetworkInterface::IsUp),
                                 iface.flags().testFlag(QNetworkInterface::IsRunning));
            info.isPhysical = (info.type == InterfaceType::Physical);
            info.displayName = QStringLiteral("%1 %2").arg(info.name, addr.toString());
            result.append(info);
        }
    }
    return result;
}

QList<NetworkInterfaceInfo> InterfaceEnumerator::candidates()
{
    QList<NetworkInterfaceInfo> all = enumerate();
    QList<NetworkInterfaceInfo> result;

    // 仅保留物理网卡
    for (const NetworkInterfaceInfo &info : all) {
        if (info.type == InterfaceType::Physical)
            result.append(info);
    }

    // 排序：有默认网关的网卡排在最前面
    const QStringList defaultIfaces = defaultGatewayInterfaces();
    std::sort(result.begin(), result.end(),
              [&defaultIfaces](const NetworkInterfaceInfo &a,
                               const NetworkInterfaceInfo &b) {
                  const bool aHasGw = defaultIfaces.contains(a.id);
                  const bool bHasGw = defaultIfaces.contains(b.id);
                  if (aHasGw != bHasGw) return aHasGw;
                  return a.name < b.name;
              });

    return result;
}

QString NetworkInterfaceInfo::cidr() const
{
    // 计算网络地址 = IP & 掩码
    const quint32 mask = (prefixLength == 0)
        ? 0 : (0xFFFFFFFF << (32 - prefixLength));
    const quint32 network = ip.toIPv4Address() & mask;
    QHostAddress netAddr(network);
    return QStringLiteral("%1/%2").arg(netAddr.toString()).arg(prefixLength);
}

} // namespace FengSui
