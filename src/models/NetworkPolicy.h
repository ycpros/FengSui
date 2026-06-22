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

// 网络模式枚举。
// 决定 TcpServer 绑定策略和 UdpDiscovery 发送策略。
// 默认 secure_lan，生产环境不推荐切换为 compat_test（14_network_model.md §3）。
enum class NetworkMode {
    secure_lan,  // 单授权网卡模式：TcpServer 仅绑定第一张匹配网卡，UdpDiscovery 仅向该网段广播
    multi_lan,   // 多网卡模式：TcpServer 绑定所有授权网卡，UdpDiscovery 向每张网卡独立发送
    compat_test  // 测试模式：放宽限制，允许绑定 0.0.0.0，仅用于本地开发和自动化测试
};

// 绑定端点描述。
// TcpServer 根据此结构决定在哪个 IP:端口上 listen()。
// 由 NetworkPolicy::getBindEndpoints() 根据当前策略和本机网卡列表生成（14_network_model.md §5）。
struct BindEndpoint {
    QString      interfaceId;   // 网卡唯一标识（QNetworkInterface::name() 返回值），如 "eth0"
    QString      interfaceName; // 网卡可读名称，如 "Ethernet"，用于设置页面展示
    QHostAddress ip;            // 绑定的 IPv4 地址，如 192.168.1.100，不可为 0.0.0.0（compat_test 除外）
    quint16      port = 0;      // 绑定端口号，如 8787，调用方负责分配
};

// 接口地址快照。
// 调用方（core 层）通过 InterfaceEnumerator 获取本机网卡列表后填入此结构。
// 调用方负责按优先级排序：有默认网关的物理网卡排在首位。
struct InterfaceAddress {
    QString      interfaceId;   // 网卡唯一标识
    QString      interfaceName; // 网卡可读名称
    QHostAddress ip;            // IPv4 地址
};

// 网络策略纯逻辑模型。
// 负责授权接口管理、CIDR 网段判定和 TcpServer 绑定端点生成。
// 不依赖任何项目内模块，可在任意线程中安全调用（所有方法为 const 或纯静态计算）。
// 成员变量由 AppSettings 加载后通过 setter 注入，不直接读写 SQLite。
class NetworkPolicy {
public:
    NetworkPolicy() = default;

    // 网络模式存取。默认 secure_lan。
    void setMode(NetworkMode mode) { m_mode = mode; }
    NetworkMode mode() const { return m_mode; }

    // 授权网卡 ID 集合存取。
    // 用户在设置页面勾选网卡后，AppSettings 将选中的 interfaceId 集合注入此处。
    void setSelectedInterfaces(const QSet<QString> &ids) { m_selectedIfaces = ids; }
    QSet<QString> selectedInterfaces() const { return m_selectedIfaces; }

    // 允许 CIDR 网段列表存取。
    // 例如 ["192.168.1.0/24", "10.0.0.0/8"]，入站连接和发现消息的发送方 IP 必须落在其中。
    void setAllowedCidrs(const QStringList &cidrs) { m_allowedCidrs = cidrs; }
    QStringList allowedCidrs() const { return m_allowedCidrs; }

    // 检查 IPv4 地址是否落在任一允许 CIDR 内。
    // 遍历所有允许 CIDR，对每个 CIDR 取网络地址与掩码，与被检地址做按位与后比较。
    // 允许列表为空时返回 false（白名单为空即拒绝一切）。
    // 仅支持 IPv4，传入 IPv6 地址直接返回 false。
    bool isAddressAllowed(const QHostAddress &address) const;

    // 检查 CIDR 网段是否完全落在任一允许 CIDR 内。
    // 计算被检 CIDR 的地址范围 [first, last]，遍历允许列表判断是否存在一个 CIDR
    // 同时覆盖 first 和 last。用于校验用户新增的网段是否已在授权范围内。
    bool isCidrAllowed(const QString &cidr) const;

    // 验证 CIDR 字符串合法性。
    // 要求格式为 "a.b.c.d/n"，IP 为合法 IPv4，前缀 n 在 [0, 32] 范围内。
    static bool isValidCidr(const QString &cidr);

    // 字符串 → 网络模式枚举。
    // 输入不区分大小写，首尾空格自动去除。无法匹配时返回 secure_lan。
    static NetworkMode modeFromString(const QString &value);

    // 网络模式枚举 → 字符串，用于持久化到 settings 表。
    static QString modeToString(NetworkMode mode);

    // 从策略配置和本机网卡列表生成 TcpServer 绑定端点。
    // port: TcpServer 监听端口。
    // ifaceIps: 调用方按优先级排序后的接口地址列表（有网关的物理网卡排首位）。
    // 过滤规则：网卡 ID 必须在授权集合中，且网卡 IP 必须落在允许 CIDR 内。
    // secure_lan 仅返回第一个匹配端点；multi_lan 返回全部；compat_test 无额外限制。
    QList<BindEndpoint> getBindEndpoints(
        quint16 port,
        const QList<InterfaceAddress> &ifaceIps) const;

    // 兼容旧调用路径：接受 QHash 输入，内部按 key 排序后委托给 List 版本。
    // 排序保证端点顺序确定性，避免 QHash 的不确定迭代顺序影响绑定行为。
    QList<BindEndpoint> getBindEndpoints(
        quint16 port,
        const QHash<QString, QPair<QString, QHostAddress>> &ifaceIps) const;

    // 将字符串形式的允许 CIDR 列表解析为 (网络地址, 前缀长度) 对。
    // UdpDiscovery 据此生成 directed broadcast 地址，向正确的子网发送 discovery 报文。
    QList<QPair<QHostAddress, int>> getAllowedSubnets() const;

private:
    // 解析单个 CIDR 字符串。
    // cidr: 输入，如 "192.168.1.100/24"，IP 部分不必是网络地址（函数自动清零主机位）。
    // netOut: 输出，纯网络地址（主机位已清零），如 192.168.1.0。
    // prefixOut: 输出，前缀长度 [0, 32]。
    // 返回 true 表示解析成功。
    // prefix=0 时掩码为 0，netOut 返回 0.0.0.0（匹配整个 IPv4 空间）。
    static bool parseCidr(const QString &cidr, QHostAddress &netOut, int &prefixOut);

    NetworkMode m_mode = NetworkMode::secure_lan;  // 网络模式，默认安全内网
    QSet<QString> m_selectedIfaces;                // 用户授权的网卡 ID 集合，由设置页面写入
    QStringList m_allowedCidrs;                    // 允许的 CIDR 网段列表，如 ["192.168.1.0/24"]
};

// ---- 以下为内联实现 ----

// 检查 IPv4 地址是否在允许 CIDR 范围内。
// 算法（14_network_model.md §4.2）：
//   步骤 1：排除非 IPv4 地址。
//   步骤 2：遍历允许 CIDR 列表，对每个 CIDR 计算掩码和网络地址。
//   步骤 3：(address & mask) == (network & mask) 时命中。
// prefix=0 时 mask=0（避免 0xFFFFFFFF << 32 的未定义行为），匹配所有地址。
inline bool NetworkPolicy::isAddressAllowed(const QHostAddress &address) const
{
    // 仅支持 IPv4，IPv6 暂不涉及局域网发现场景
    if (address.protocol() != QAbstractSocket::IPv4Protocol) return false;

    const quint32 ip = address.toIPv4Address();

    for (const QString &cidr : m_allowedCidrs) {
        QHostAddress network;
        int prefix = 0;
        if (!parseCidr(cidr, network, prefix)) continue;  // 跳过非法 CIDR，不中断遍历

        // prefix=0 时 mask=0，使 (ip & 0) == 0 恒成立（/0 覆盖所有 IPv4 地址）
        const quint32 mask  = (prefix == 0) ? 0 : (0xFFFFFFFFu << (32 - prefix));
        const quint32 netIp = network.toIPv4Address();

        // 被检地址的网络部分与允许网段的网络部分相同时命中
        if ((ip & mask) == (netIp & mask)) return true;
    }

    return false;
}

// 检查一个 CIDR 网段是否完全被任一允许 CIDR 覆盖。
// 算法：
//   步骤 1：解析被检 CIDR，计算其地址范围 [first, last]。
//   步骤 2：遍历允许 CIDR 列表，检查是否存在一个 CIDR 同时覆盖 first 和 last。
// 用途：校验用户新添加的网段是否已在授权范围内，避免重复配置。
inline bool NetworkPolicy::isCidrAllowed(const QString &cidr) const
{
    QHostAddress network;
    int prefix = 0;
    if (!parseCidr(cidr, network, prefix)) return false;

    // 计算被检网段的地址范围
    // mask: 网络位掩码，如 /24 → 0xFFFFFF00
    // ~mask 取反后高位可能含 1（mask 是无符号 32 位），需要 & 0xFFFFFFFF 截断
    const quint32 mask  = (prefix == 0) ? 0 : (0xFFFFFFFFu << (32 - prefix));
    const quint32 first = network.toIPv4Address() & mask;          // 主机位全 0，网段起始地址
    const quint32 last  = first | (~mask & 0xFFFFFFFFu);           // 主机位全 1，网段广播地址

    for (const QString &allowedCidr : m_allowedCidrs) {
        QHostAddress allowedNetwork;
        int allowedPrefix = 0;
        if (!parseCidr(allowedCidr, allowedNetwork, allowedPrefix)) continue;

        const quint32 allowedMask = (allowedPrefix == 0)
            ? 0 : (0xFFFFFFFFu << (32 - allowedPrefix));
        const quint32 allowedFirst = allowedNetwork.toIPv4Address() & allowedMask;

        // 被检网段的起始和结束地址必须落在同一个允许 CIDR 内
        if ((first & allowedMask) == allowedFirst
            && (last & allowedMask) == allowedFirst) {
            return true;
        }
    }

    return false;
}

// CIDR 合法性校验，委托 parseCidr，仅关注格式是否正确。
inline bool NetworkPolicy::isValidCidr(const QString &cidr)
{
    QHostAddress ignored;
    int prefix = 0;
    return parseCidr(cidr, ignored, prefix);
}

// 字符串 → 网络模式枚举。
// 输入先 trim → toLower，不区分大小写。未知值默认返回 secure_lan。
inline NetworkMode NetworkPolicy::modeFromString(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("multi_lan")) {
        return NetworkMode::multi_lan;
    }
    if (normalized == QStringLiteral("compat_test")) {
        return NetworkMode::compat_test;
    }
    return NetworkMode::secure_lan;  // 默认：包括 "secure_lan" 及任何无法匹配的输入
}

// 网络模式枚举 → 持久化字符串。
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

// 从策略配置和本机网卡列表生成 TcpServer 绑定端点（List 版本，主实现）。
// 过滤规则（两项必须同时满足）：
//   1. 网卡 ID 在授权集合中（用户在设置页面勾选了该网卡）
//   2. 网卡 IP 落在允许 CIDR 内（安全策略校验）
// 模式差异：
//   secure_lan: 仅返回第一个匹配端点（break 提前退出），最小暴露面
//   multi_lan:  返回所有匹配端点，每个网卡独立监听
//   compat_test: 过滤规则不变，无额外限制
inline QList<BindEndpoint> NetworkPolicy::getBindEndpoints(
    quint16 port,
    const QList<InterfaceAddress> &ifaceIps) const
{
    QList<BindEndpoint> endpoints;

    for (const InterfaceAddress &iface : ifaceIps) {
        // 过滤 1：网卡未被用户授权
        if (!m_selectedIfaces.contains(iface.interfaceId)) continue;

        // 过滤 2：网卡 IP 不在允许网段内
        if (!isAddressAllowed(iface.ip)) continue;

        BindEndpoint ep;
        ep.interfaceId   = iface.interfaceId;
        ep.interfaceName = iface.interfaceName;
        ep.ip            = iface.ip;
        ep.port          = port;
        endpoints.append(ep);

        // secure_lan 模式：仅暴露一张网卡，首个匹配后即停止
        if (m_mode == NetworkMode::secure_lan) break;
    }

    return endpoints;
}

// 从 QHash 输入生成绑定端点（兼容版本）。
// 内部按 key 字典序排序后委托给 List 版本。
// 排序的必要性：QHash 迭代顺序不确定，直接传入会导致端点顺序每次不同，
// 影响 TcpServer 绑定行为和调试可复现性。
inline QList<BindEndpoint> NetworkPolicy::getBindEndpoints(
    quint16 port,
    const QHash<QString, QPair<QString, QHostAddress>> &ifaceIps) const
{
    QList<QString> keys = ifaceIps.keys();
    std::sort(keys.begin(), keys.end());  // 字典序排序，保证确定性

    QList<InterfaceAddress> ordered;
    for (const QString &key : keys) {
        InterfaceAddress iface;
        iface.interfaceId   = key;
        iface.interfaceName = ifaceIps.value(key).first;
        iface.ip            = ifaceIps.value(key).second;
        ordered.append(iface);
    }

    return getBindEndpoints(port, ordered);  // 委托给主版本
}

// 解析允许 CIDR 列表为 (网络地址, 前缀长度) 对。
// 跳过解析失败的非法 CIDR（不会中断遍历）。
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

// 解析单个 CIDR 字符串为网络地址和前缀长度。
// 输入示例："192.168.1.100/24"。
// 处理流程（06_protocol.md §2）：
//   步骤 1：按 '/' 分割，提取 IP 部分和前缀部分。
//   步骤 2：解析 IP 为 QHostAddress，必须为合法 IPv4。
//   步骤 3：解析前缀为 int，范围 [0, 32]。
//   步骤 4：计算掩码并清零主机位，得到纯网络地址。
// 注意：输入 IP 不必是网络地址（如 192.168.1.100），函数自动做 & mask 修正。
inline bool NetworkPolicy::parseCidr(const QString &cidr,
                                     QHostAddress &netOut,
                                     int &prefixOut)
{
    // 步骤 1：按 '/' 分割
    const int slashPos = cidr.indexOf(QLatin1Char('/'));
    if (slashPos < 0) return false;  // 无 '/' → 非法格式

    const QString ipPart     = cidr.left(slashPos).trimmed();
    const QString prefixPart = cidr.mid(slashPos + 1).trimmed();

    // 步骤 2：解析 IP 部分
    QHostAddress addr(ipPart);
    if (addr.isNull() || addr.protocol() != QAbstractSocket::IPv4Protocol) return false;

    // 步骤 3：解析前缀长度
    bool ok = false;
    const int prefix = prefixPart.toInt(&ok);
    if (!ok || prefix < 0 || prefix > 32) return false;

    // 步骤 4：计算掩码并清零主机位，得到纯网络地址
    // prefix=0 时 mask=0 是特例：0xFFFFFFFFu << 32 对 32 位整数是未定义行为，必须单独处理
    const quint32 mask    = (prefix == 0) ? 0 : (0xFFFFFFFFu << (32 - prefix));
    const quint32 network = addr.toIPv4Address() & mask;  // 清零主机位

    netOut    = QHostAddress(network);  // 纯网络地址，如 192.168.1.0
    prefixOut = prefix;
    return true;
}

} // namespace FengSui
