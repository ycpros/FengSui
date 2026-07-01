// PeerInfo.h
// 局域网设备/联系人信息结构体。
// 纯数据结构，不含业务逻辑，可被所有层 include。

#pragma once

#include <QObject>       // Q_GADGET / Q_PROPERTY 需要（提供反射元数据给 QML）
#include <QString>
#include <QDateTime>

namespace FengSui {

// 对等设备信息。
// peerId 由 SHA-256(主机名+MAC+随机盐) 生成，全网唯一。
// 加 Q_GADGET + Q_PROPERTY 让 QML 能反射字段；仍是纯值类型，signal/slot 按值传递不变。
struct PeerInfo {
    Q_GADGET
    Q_PROPERTY(QString peerId MEMBER peerId)
    Q_PROPERTY(QString displayName MEMBER displayName)
    Q_PROPERTY(QString deviceName MEMBER deviceName)
    Q_PROPERTY(QString ip MEMBER ip)
    Q_PROPERTY(quint16 port MEMBER port)
    Q_PROPERTY(QString os MEMBER os)
    Q_PROPERTY(bool online MEMBER online)
    Q_PROPERTY(QDateTime lastSeenAt MEMBER lastSeenAt)
    Q_PROPERTY(bool shareEnabled MEMBER shareEnabled)
    Q_PROPERTY(QString version MEMBER version)
public:
    QString peerId;          // 唯一标识，如 "a1b2c3d4"
    QString displayName;     // 用户设置的昵称，如 "Lily-PC"
    QString deviceName;      // 系统主机名，如 "DESKTOP-001"
    QString ip;              // IPv4 地址，如 "192.168.1.20"
    quint16 port = 0;        // TCP 监听端口，默认 8787
    QString os;              // 操作系统，"windows" / "linux" / "macos"
    bool online = false;     // 当前是否在线
    QDateTime lastSeenAt;    // 最后一次在线时间
    bool shareEnabled = false; // 是否启用了共享目录
    QString version;         // 客户端版本号，如 "0.1.0"
};

} // namespace FengSui
