// DownloadLog.h
// 下载记录：记录从其他设备共享目录下载文件的操作日志。

#pragma once

#include <QObject>       // Q_GADGET / Q_PROPERTY 需要
#include <QString>
#include <QDateTime>

namespace FengSui {

// 共享文件下载日志
struct DownloadLog {
    Q_GADGET
    Q_PROPERTY(QString logId MEMBER logId)
    Q_PROPERTY(QString peerId MEMBER peerId)
    Q_PROPERTY(QString shareId MEMBER shareId)
    Q_PROPERTY(QString remotePath MEMBER remotePath)
    Q_PROPERTY(QString localPath MEMBER localPath)
    Q_PROPERTY(QString fileName MEMBER fileName)
    Q_PROPERTY(qint64 fileSize MEMBER fileSize)
    Q_PROPERTY(QDateTime downloadedAt MEMBER downloadedAt)
    Q_PROPERTY(bool success MEMBER success)
public:
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
