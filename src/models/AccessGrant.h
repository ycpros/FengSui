// AccessGrant.h
// 共享访问授权记录：记录其他设备对本机共享目录的访问权限。

#pragma once

#include <QString>
#include <QDateTime>

namespace FengSui {

// 共享访问授权
struct AccessGrant {
    QString peerId;           // 被授权设备的 peerId
    QString shareId;          // 被授权的共享目录 shareId
    QDateTime grantedAt;
    bool remember = false;    // 是否记住选择（同设备后续访问不再弹窗）
};

} // namespace FengSui
