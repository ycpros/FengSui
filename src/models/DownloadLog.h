// DownloadLog.h
// 下载记录：记录从其他设备共享目录下载文件的操作日志。

#pragma once

#include <QString>
#include <QDateTime>

namespace FengSui {

// 共享文件下载日志
struct DownloadLog {
    QString logId;       // UUID
    QString peerId;      // 来源设备 peerId
    QString shareId;     // 来源共享目录 shareId
    QString remotePath;  // 源文件远程路径
    QString localPath;   // 本地保存路径
    QString fileName;
    qint64 fileSize = 0;
    QDateTime downloadedAt;
    bool success = false;
};

} // namespace FengSui
