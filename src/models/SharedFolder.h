// SharedFolder.h
// 共享目录数据结构：用户指定对外共享的本地目录。

#pragma once

#include <QString>

namespace FengSui {

// 共享目录配置。
// 存储在 shared_folders 表中，ShareService 据此提供远程浏览与下载服务。
struct SharedFolder {
    QString shareId;       // UUID
    QString localPath;     // 本地绝对路径，如 "D:/Projects/design"
    QString displayName;   // 对外显示名称，默认取目录名
    bool isActive = true;  // 是否启用
};

} // namespace FengSui
