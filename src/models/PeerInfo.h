// PeerInfo.h
// 局域网设备/联系人信息结构体。
// 纯数据结构，不含业务逻辑，可被所有层 include。

#pragma once

#include <QString>
#include <QDateTime>

namespace FengSui {

// 对等设备信息。
// peerId 由 SHA-256(主机名+MAC+随机盐) 生成，全网唯一。
struct PeerInfo {
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
