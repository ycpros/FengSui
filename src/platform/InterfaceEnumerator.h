// InterfaceEnumerator.h
// 本机网卡枚举与分类，为 NetworkPolicy 提供候选网卡列表。
// 按 14_network_model.md §4.1 规则过滤。
#pragma once

#include "models/ModelEnums.h"  // InterfaceType 枚举定义 + QML 反射注册
#include <QHostAddress>
#include <QList>
#include <QObject>              // Q_GADGET / Q_PROPERTY 需要
#include <QString>
#include <QStringList>

namespace FengSui {

// 本机网卡信息
struct NetworkInterfaceInfo {
    Q_GADGET
    Q_PROPERTY(QString id MEMBER id)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString displayName MEMBER displayName)
    Q_PROPERTY(int prefixLength MEMBER prefixLength)
    Q_PROPERTY(bool isPhysical MEMBER isPhysical)
    Q_PROPERTY(InterfaceType type MEMBER type)
    // 只读派生属性：CIDR 字符串（供 QML 直接绑定显示）
    Q_PROPERTY(QString cidr READ cidr CONSTANT)
public:
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

    // 校正持久化的授权网卡 ID，使其与当前候选网卡快照保持一致。
    // selectedIds: 设置中保存的授权网卡 ID，可包含空白、重复项或已失效 ID。
    // candidateInterfaces: 已按主网卡优先级排序的当前物理候选网卡快照。
    // 返回值：按 selectedIds 原有顺序保留仍有效的唯一物理网卡 ID；如果没有任何
    // 有效选择，则回退到 candidateInterfaces 中首个有效物理网卡。没有物理候选时
    // 返回空列表。
    // 该函数只处理传入快照，不重新枚举系统网卡，便于调用方保证一致性并进行单元测试。
    static QStringList resolveSelectedInterfaceIds(
        const QStringList &selectedIds,
        const QList<NetworkInterfaceInfo> &candidateInterfaces);

    // 根据授权网卡的当前 IPv4 地址和前缀长度推导允许网段 CIDR。
    // selectedIds: 已校正的授权网卡 ID；空列表表示没有授权网卡。
    // interfaces: 当前完整网卡地址快照，同一网卡可以包含多个 IPv4 地址条目。
    // 返回值：按 interfaces 遍历顺序返回所有匹配物理网卡的唯一 CIDR。非物理网卡、
    // 非 IPv4 地址和非法前缀长度会被忽略；不同地址落在同一子网时只保留一个 CIDR。
    // 该函数不读取 allowed_cidrs 缓存，确保结果始终由当前网卡状态决定。
    static QStringList cidrsForSelectedInterfaces(
        const QStringList &selectedIds,
        const QList<NetworkInterfaceInfo> &interfaces);

private:
    // 根据名称和地址对网卡分类
    static InterfaceType classify(const QString &name,
                                  const QHostAddress &address,
                                  bool isUp,
                                  bool isRunning);
};

} // namespace FengSui
