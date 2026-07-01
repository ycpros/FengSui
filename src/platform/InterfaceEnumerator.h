// InterfaceEnumerator.h
// 本机网卡枚举与分类，为 NetworkPolicy 提供候选网卡列表。
// 按 14_network_model.md §4.1 规则过滤。
#pragma once

#include "models/ModelEnums.h"  // InterfaceType 枚举定义 + QML 反射注册
#include <QHostAddress>
#include <QList>
#include <QObject>              // Q_GADGET / Q_PROPERTY 需要
#include <QString>

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

private:
    // 根据名称和地址对网卡分类
    static InterfaceType classify(const QString &name,
                                  const QHostAddress &address,
                                  bool isUp,
                                  bool isRunning);
};

} // namespace FengSui
