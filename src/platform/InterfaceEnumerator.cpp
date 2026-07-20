// InterfaceEnumerator.cpp
// 本机网卡枚举与分类实现。
#include "platform/InterfaceEnumerator.h"

#include <QAbstractSocket>
#include <QList>
#include <QNetworkAddressEntry>
#include <QNetworkInterface>
#include <QSet>
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

QStringList InterfaceEnumerator::resolveSelectedInterfaceIds(
    const QStringList &selectedIds,
    const QList<NetworkInterfaceInfo> &candidateInterfaces)
{
    // 候选列表可能因一张网卡拥有多个 IPv4 地址而出现重复 ID。先构造集合，后续校正
    // 只关心网卡是否仍存在且仍被分类为物理网卡。
    QSet<QString> availableIds;
    for (const NetworkInterfaceInfo &iface : candidateInterfaces) {
        const QString id = iface.id.trimmed();
        if (iface.isPhysical && !id.isEmpty()) {
            availableIds.insert(id);
        }
    }

    // 保留用户原有选择顺序，同时清除空白、失效、非物理和重复的网卡 ID。保留顺序
    // 很重要：secure_lan 模式会使用首个匹配端点，校正不应无故改变用户优先级。
    QStringList resolved;
    for (const QString &selectedId : selectedIds) {
        const QString id = selectedId.trimmed();
        if (availableIds.contains(id) && !resolved.contains(id)) {
            resolved.append(id);
        }
    }

    // 空选择既可能来自首次启动，也可能表示原授权网卡已全部消失。候选列表已经按照
    // 主网卡优先级排序，因此选择首个物理候选即可复用现有的主网卡判定规则。
    if (resolved.isEmpty()) {
        for (const NetworkInterfaceInfo &iface : candidateInterfaces) {
            const QString id = iface.id.trimmed();
            if (iface.isPhysical && !id.isEmpty()) {
                resolved.append(id);
                break;
            }
        }
    }

    return resolved;
}

QStringList InterfaceEnumerator::cidrsForSelectedInterfaces(
    const QStringList &selectedIds,
    const QList<NetworkInterfaceInfo> &interfaces)
{
    // 使用集合匹配授权 ID，避免重复选择导致同一网卡被重复处理。
    QSet<QString> selected;
    for (const QString &selectedId : selectedIds) {
        const QString id = selectedId.trimmed();
        if (!id.isEmpty()) {
            selected.insert(id);
        }
    }

    QStringList cidrs;
    for (const NetworkInterfaceInfo &iface : interfaces) {
        // interfaces 是完整枚举结果，可能包含虚拟、公网或状态异常的网卡。即使调用方
        // 传入了异常 ID，也只能由当前仍有效的物理 IPv4 地址生成允许网段。
        if (!iface.isPhysical
            || !selected.contains(iface.id)
            || iface.ip.protocol() != QAbstractSocket::IPv4Protocol
            || iface.prefixLength < 0
            || iface.prefixLength > 32) {
            continue;
        }

        const QString cidr = iface.cidr();
        // 多个地址可能落在同一子网，例如 DHCP 地址切换期间同时存在旧、新地址条目。
        // 保持首次出现顺序并去重，使设置缓存和诊断输出稳定可读。
        if (!cidr.isEmpty() && !cidrs.contains(cidr)) {
            cidrs.append(cidr);
        }
    }

    return cidrs;
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
