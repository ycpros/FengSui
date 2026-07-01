// TransferTask.h
// 文件传输任务数据结构：记录发送或接收的文件传输状态。

#pragma once

#include "models/ModelEnums.h"  // TransferDirection / TransferStatus 枚举定义 + QML 反射注册
#include <QObject>              // Q_GADGET / Q_PROPERTY 需要
#include <QMetaType>
#include <QString>
#include <QDateTime>

namespace FengSui {

// 文件传输任务。
// 存储在 transfer_tasks 表中，传输中心页面从此读取。
struct TransferTask {
    Q_GADGET
    Q_PROPERTY(QString transferId MEMBER transferId)
    Q_PROPERTY(TransferDirection direction MEMBER direction)
    Q_PROPERTY(QString peerId MEMBER peerId)
    Q_PROPERTY(QString fileName MEMBER fileName)
    Q_PROPERTY(QString filePath MEMBER filePath)
    Q_PROPERTY(qint64 fileSize MEMBER fileSize)
    Q_PROPERTY(qint64 transferredBytes MEMBER transferredBytes)
    Q_PROPERTY(QString sha256 MEMBER sha256)
    Q_PROPERTY(TransferStatus status MEMBER status)
    Q_PROPERTY(QString errorMessage MEMBER errorMessage)
    Q_PROPERTY(QDateTime createdAt MEMBER createdAt)
    Q_PROPERTY(QDateTime completedAt MEMBER completedAt)
public:
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
