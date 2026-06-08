// TransferTask.h
// 文件传输任务数据结构：记录发送或接收的文件传输状态。

#pragma once

#include <QMetaType>
#include <QString>
#include <QDateTime>

namespace FengSui {

// 传输方向
enum class TransferDirection {
    Upload,     // 我发出
    Download    // 我接收
};

// 传输状态
enum class TransferStatus {
    Waiting,        // 等待对方接受
    Rejected,       // 对方拒绝
    Transferring,   // 传输中
    Completed,      // 传输完成
    Failed,         // 传输失败
    Cancelled       // 已取消
};

// 文件传输任务。
// 存储在 transfer_tasks 表中，传输中心页面从此读取。
struct TransferTask {
    QString transferId;          // UUID
    TransferDirection direction;
    QString peerId;              // 对方 peerId
    QString fileName;
    QString filePath;            // 本地文件路径（发出为源，接收为下载目标）
    qint64 fileSize = 0;
    qint64 transferredBytes = 0; // 已传输字节
    QString sha256;              // 文件校验
    TransferStatus status = TransferStatus::Waiting;
    QString errorMessage;        // 失败原因
    QDateTime createdAt;
    QDateTime completedAt;
};

} // namespace FengSui

Q_DECLARE_METATYPE(FengSui::TransferTask)
