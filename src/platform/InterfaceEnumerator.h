// InterfaceEnumerator.h
// 本机网卡枚举与分类，为 NetworkPolicy 提供候选网卡列表。
// 按 14_network_model.md §4.1 规则过滤。
#pragma once

#include <QHostAddress>
#include <QList>
#include <QString>

namespace FengSui {

// 网卡类型分类
enum class InterfaceType {
    Physical,  // 物理网卡（Ethernet、Wi-Fi）
    Virtual,   // 虚拟网卡（VMware、Hyper-V、WSL、ZeroTier 等）
    Loopback,  // 回环接口
    LinkLocal, // 链路本地地址（169.254.0.0/16）
    Public,    // 公网地址（非 RFC1918）
    Unknown    // 无法分类
};

// 本机网卡信息
struct NetworkInterfaceInfo {
    QString       id;           // 网卡唯一标识（QNetworkInterface::name()）
    QString       name;         // 网卡可读名称（如 "Ethernet"）
    QString       displayName;  // 显示名称（如 "Ethernet 192.168.10.25"）
    QHostAddress  ip;           // IPv4 地址
    int           prefixLength = 24;  // 子网前缀长度
    bool          isPhysical = false; // 是否为物理网卡
    InterfaceType type = InterfaceType::Unknown;

    // 生成该网卡所在网段的 CIDR 字符串（如 "192.168.10.0/24"）
    QString cidr() const;
};

// 本机网卡枚举器
class InterfaceEnumerator {
public:
    // 枚举本机所有 IPv4 网卡（不做过滤，仅排除回环）
    static QList<NetworkInterfaceInfo> enumerate();

    // 枚举候选授权网卡（按 14_network_model.md §4.1 过滤）
    // 排除：loopback、链路本地、未连接、虚拟网卡、公网地址
    // 排序：有默认网关的物理网卡排在第一位
    static QList<NetworkInterfaceInfo> candidates();

private:
    // 根据名称和地址对网卡分类
    static InterfaceType classify(const QString &name,
                                  const QHostAddress &address,
                                  bool isUp,
                                  bool isRunning);
};

} // namespace FengSui
