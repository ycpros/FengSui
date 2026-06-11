// CourierService.cpp
// 文件传输编排服务实现：传输生命周期管理、分块收发、SHA-256 校验。

#include "core/CourierService.h"
#include "app/AppSettings.h"
#include "core/SignalService.h"
#include "network/Protocol.h"
#include "network/TcpConnection.h"
#include "storage/TransferRepository.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTimer>
#include <QUuid>

namespace FengSui {

namespace {

// 协议约定的传输块大小。TcpConnection 也按 8MB 作为单块上限。
constexpr quint32 kTransferChunkSize = 8 * 1024 * 1024;

// SHA-256 计算缓冲，独立于网络块大小，避免计算 hash 时占用过大临时内存。
constexpr int kHashBufferSize = 1024 * 1024;

// 发送完一块后通过 QTimer::singleShot(0, ...) 让出事件循环，
// 保证 UI 刷新和进度更新不被大文件传输阻塞。
constexpr int kChunkSendYieldMs = 0;

QString sanitizedFileName(const QString& fileName)
{
    QString normalized = fileName;
    normalized.replace(QLatin1Char('\\'), QLatin1Char('/'));

    QString safeName = QFileInfo(normalized).fileName().trimmed();
    if (safeName.isEmpty() || safeName == QStringLiteral(".")
        || safeName == QStringLiteral("..")) {
        safeName = QStringLiteral("fengsui-download");
    }
    return safeName;
}

QString uniqueFilePath(const QString& directoryPath, const QString& fileName)
{
    const QDir dir(directoryPath);
    const QString safeName = sanitizedFileName(fileName);
    const QFileInfo safeInfo(safeName);
    const QString baseName = safeInfo.completeBaseName().isEmpty()
        ? safeInfo.fileName()
        : safeInfo.completeBaseName();
    const QString suffix = safeInfo.suffix();

    QString candidate = dir.filePath(safeName);
    int index = 1;
    while (QFileInfo::exists(candidate)) {
        const QString numberedName = suffix.isEmpty()
            ? QStringLiteral("%1 (%2)").arg(baseName).arg(index)
            : QStringLiteral("%1 (%2).%3").arg(baseName).arg(index).arg(suffix);
        candidate = dir.filePath(numberedName);
        ++index;
    }
    return candidate;
}

} // namespace

CourierService::CourierService(QObject* parent)
    : QObject(parent)
{
}

CourierService::~CourierService()
{
    // 清理活跃传输上下文（关闭文件、释放资源）
    const QList<QString> ids = m_activeTransfers.keys();
    for (const QString& id : ids) {
        TransferContext& ctx = m_activeTransfers[id];
        if (ctx.file) {
            ctx.file->close();
            delete ctx.file;
            ctx.file = nullptr;
        }
        if (ctx.hasher) {
            delete ctx.hasher;
            ctx.hasher = nullptr;
        }
    }
    m_activeTransfers.clear();
}

// ---- 依赖注入 ----

void CourierService::setSignalService(SignalService* service)
{
    m_signalService = service;
}

void CourierService::setAppSettings(AppSettings* settings)
{
    m_settings = settings;
}

void CourierService::setTransferRepository(TransferRepository* repo)
{
    m_transferRepo = repo;
}

void CourierService::setLocalPeerId(const QString& peerId)
{
    m_localPeerId = peerId;
}

// ---- 传输操作 ----

QString CourierService::sendFile(const PeerInfo& peer, const QString& filePath)
{
    const QString transferId = generateTransferId();

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        qWarning() << "sendFile: file does not exist or is not a regular file:" << filePath;
        emit transferFailed(transferId,
                            QStringLiteral("文件不存在: %1").arg(filePath));
        return transferId;
    }

    const qint64 fileSize = fileInfo.size();
    const QString fileName = fileInfo.fileName();

    // 计算源文件 SHA-256（同步，仅在发送前计算一次）
    const QString sha256 = computeFileSha256(filePath);

    // 构建传输上下文
    TransferContext ctx;
    ctx.task.transferId = transferId;
    ctx.task.direction = TransferDirection::Upload;
    ctx.task.peerId = peer.peerId;
    ctx.task.fileName = fileName;
    ctx.task.filePath = filePath;
    ctx.task.fileSize = fileSize;
    ctx.task.sha256 = sha256;
    ctx.task.status = TransferStatus::Waiting;
    ctx.task.createdAt = QDateTime::currentDateTimeUtc();
    ctx.peer = peer;
    ctx.isOutgoing = true;
    ctx.chunkSize = kTransferChunkSize;

    // 打开发送文件
    ctx.file = new QFile(filePath);
    if (!ctx.file->open(QIODevice::ReadOnly)) {
        qWarning() << "sendFile: cannot open file for reading:" << filePath;
        emit transferFailed(transferId,
                            QStringLiteral("无法打开文件: %1").arg(ctx.file->errorString()));
        delete ctx.file;
        return transferId;
    }

    // 持久化到 SQLite
    if (m_transferRepo) {
        m_transferRepo->saveTask(ctx.task);
    }

    // 注册到活跃传输映射
    m_activeTransfers.insert(transferId, ctx);

    // 向对方发送 transfer.request
    if (m_signalService) {
        const QJsonObject request = Protocol::buildTransferRequest(
            transferId, m_localPeerId, peer.peerId, fileName, fileSize, sha256,
            ctx.chunkSize, false);

        // 通过 SignalService 发送 JSON 控制消息并获取连接
        TcpConnection* conn = m_signalService->sendJsonMessage(peer, request);
        if (!conn) {
            qWarning() << "sendFile: no connection to peer" << peer.peerId
                       << ", transfer.request will be queued";
        }
    } else {
        qWarning() << "sendFile: SignalService not injected";
    }

    qInfo() << "File transfer initiated:" << transferId << fileName << fileSize << "bytes";
    return transferId;
}

QString CourierService::sendFolder(const PeerInfo& peer, const QString& dirPath)
{
    // TODO(ycpros): 实现文件夹递归打包传输（Task 011 验收标准 F007）。
    // 当前先标记未实现，返回空 transferId。
    qWarning() << "sendFolder not yet implemented, dirPath:" << dirPath;

    const QString transferId = generateTransferId();
    emit transferFailed(transferId,
                        QStringLiteral("文件夹传输暂未实现"));
    return transferId;
}

void CourierService::acceptTransfer(const QString& transferId)
{
    auto it = m_activeTransfers.find(transferId);
    if (it == m_activeTransfers.end()) {
        qWarning() << "acceptTransfer: unknown transfer" << transferId;
        return;
    }

    TransferContext& ctx = it.value();
    if (ctx.isOutgoing) {
        qWarning() << "acceptTransfer: transfer" << transferId << "is outgoing, not incoming";
        return;
    }

    QFileInfo targetInfo(ctx.task.filePath);
    QDir targetDir = targetInfo.absoluteDir();
    if (!targetDir.exists() && !QDir().mkpath(targetDir.absolutePath())) {
        const QString error = QStringLiteral("无法创建下载目录: %1")
                                  .arg(targetDir.absolutePath());
        sendTransferError(transferId, QStringLiteral("DISK_ERROR"), error);
        finishTransfer(transferId, false, error);
        return;
    }
    if (QFileInfo::exists(ctx.task.filePath)) {
        ctx.task.filePath = uniqueFilePath(targetDir.absolutePath(),
                                           ctx.task.fileName);
        if (m_transferRepo) {
            m_transferRepo->saveTask(ctx.task);
        }
    }

    ctx.file = new QFile(ctx.task.filePath);
    if (!ctx.file->open(QIODevice::WriteOnly)) {
        const QString error = QStringLiteral("无法保存文件: %1")
                                  .arg(ctx.file->errorString());
        delete ctx.file;
        ctx.file = nullptr;
        sendTransferError(transferId, QStringLiteral("DISK_ERROR"), error);
        finishTransfer(transferId, false, error);
        return;
    }

    // 发送 transfer.accept
    TcpConnection* conn = findConnectionForTransfer(transferId);
    if (conn && conn->isConnected()) {
        const QJsonObject accept = Protocol::buildTransferAccept(transferId);
        conn->sendMessage(accept);
    } else {
        const QString error = QStringLiteral("对端连接已断开");
        qWarning() << "acceptTransfer: no connection for transfer" << transferId;
        finishTransfer(transferId, false, error);
        return;
    }

    // 更新状态为传输中
    ctx.task.status = TransferStatus::Transferring;
    ctx.task.transferredBytes = 0;
    ctx.hasher = new QCryptographicHash(QCryptographicHash::Sha256);

    if (m_transferRepo) {
        m_transferRepo->updateStatus(transferId, TransferStatus::Transferring);
    }

    qInfo() << "Transfer accepted by local user:" << transferId;
}

void CourierService::rejectTransfer(const QString& transferId, const QString& reason)
{
    auto it = m_activeTransfers.find(transferId);
    if (it == m_activeTransfers.end()) {
        qWarning() << "rejectTransfer: unknown transfer" << transferId;
        return;
    }

    TransferContext& ctx = it.value();

    // 发送 transfer.reject
    TcpConnection* conn = findConnectionForTransfer(transferId);
    if (conn && conn->isConnected()) {
        const QJsonObject reject = Protocol::buildTransferReject(transferId, reason);
        conn->sendMessage(reject);
    }

    // 更新状态并清理
    const QString rejectMessage = reason.isEmpty()
        ? QStringLiteral("已拒绝")
        : QStringLiteral("已拒绝: %1").arg(reason);
    finishTransfer(transferId, false, rejectMessage);
}

void CourierService::cancelTransfer(const QString& transferId)
{
    auto it = m_activeTransfers.find(transferId);
    if (it == m_activeTransfers.end()) {
        qWarning() << "cancelTransfer: unknown transfer" << transferId;
        return;
    }

    // 发送 transfer.error（告知对方传输已取消）
    TcpConnection* conn = findConnectionForTransfer(transferId);
    if (conn && conn->isConnected()) {
        const QJsonObject error = Protocol::buildTransferError(
            transferId,
            QStringLiteral("TRANSFER_CANCELLED"),
            QStringLiteral("传输已被取消"));
        conn->sendMessage(error);
    }

    finishTransfer(transferId, false, QStringLiteral("传输已取消"));
}

void CourierService::handlePeerDisconnected(const QString& peerId)
{
    if (peerId.trimmed().isEmpty()) {
        return;
    }

    const QList<QString> ids = m_activeTransfers.keys();
    for (const QString& transferId : ids) {
        auto it = m_activeTransfers.find(transferId);
        if (it == m_activeTransfers.end()) {
            continue;
        }
        if (it.value().peer.peerId == peerId) {
            finishTransfer(transferId, false, QStringLiteral("对端连接已断开"));
        }
    }
}

// ---- 消息入口（由 SignalService 调用） ----

void CourierService::handleTransferMessage(TcpConnection* connection,
                                           const QJsonObject& message)
{
    if (!connection) {
        qWarning() << "handleTransferMessage called with null connection";
        return;
    }

    const QString type = Protocol::messageType(message);

    if (type == QStringLiteral("transfer.request")) {
        handleTransferRequest(connection, message);
    } else if (type == QStringLiteral("transfer.accept")) {
        handleTransferAccept(message);
    } else if (type == QStringLiteral("transfer.reject")) {
        handleTransferReject(message);
    } else if (type == QStringLiteral("transfer.complete")) {
        handleTransferComplete(message);
    } else if (type == QStringLiteral("transfer.error")) {
        handleTransferError(message);
    } else {
        qWarning() << "Unknown transfer message type:" << type;
    }
}

void CourierService::handleChunkData(const QString& transferId,
                                     quint32 chunkIndex,
                                     const QByteArray& data)
{
    auto it = m_activeTransfers.find(transferId);
    if (it == m_activeTransfers.end()) {
        qWarning() << "handleChunkData: unknown transfer" << transferId;
        return;
    }

    TransferContext& ctx = it.value();
    if (ctx.isOutgoing) {
        qWarning() << "handleChunkData: received chunk for outgoing transfer" << transferId;
        return;
    }

    if (!ctx.file || !ctx.file->isOpen()) {
        qWarning() << "handleChunkData: file not open for transfer" << transferId;
        sendTransferError(transferId,
                          QStringLiteral("TRANSFER_NOT_READY"),
                          QStringLiteral("接收文件尚未准备好"));
        finishTransfer(transferId, false, QStringLiteral("接收文件尚未准备好"));
        return;
    }

    if (chunkIndex != ctx.nextChunkIndex) {
        const QString error = QStringLiteral("数据块顺序错误: 期望 %1，收到 %2")
                                  .arg(ctx.nextChunkIndex)
                                  .arg(chunkIndex);
        sendTransferError(transferId, QStringLiteral("CHUNK_ORDER_ERROR"), error);
        finishTransfer(transferId, false, error);
        return;
    }

    // 计算写入偏移量
    const qint64 offset = static_cast<qint64>(chunkIndex) * ctx.chunkSize;

    // 写入数据到文件
    ctx.file->seek(offset);
    const qint64 bytesWritten = ctx.file->write(data);
    if (bytesWritten != data.size()) {
        qWarning() << "handleChunkData: short write for transfer" << transferId
                   << "expected" << data.size() << "wrote" << bytesWritten;
        sendTransferError(transferId,
                          QStringLiteral("DISK_ERROR"),
                          QStringLiteral("写入文件失败: %1").arg(ctx.file->errorString()));
        finishTransfer(transferId, false,
                       QStringLiteral("写入文件失败: %1").arg(ctx.file->errorString()));
        return;
    }

    // 更新 SHA-256 累加器
    if (ctx.hasher) {
        ctx.hasher->addData(data);
    }

    // 更新进度
    ctx.task.transferredBytes += data.size();
    ctx.nextChunkIndex++;
    if (m_transferRepo) {
        m_transferRepo->updateProgress(transferId, ctx.task.transferredBytes);
    }

    emit transferProgress(transferId, ctx.task.transferredBytes, ctx.task.fileSize);
}

// ---- Private Handlers ----

void CourierService::handleTransferRequest(TcpConnection* connection,
                                           const QJsonObject& message)
{
    const QString transferId = message.value(QStringLiteral("transfer_id")).toString();
    const QString fromPeerId = message.value(QStringLiteral("from")).toString();
    const QString fileName = message.value(QStringLiteral("file_name")).toString();
    const qint64 fileSize = static_cast<qint64>(
        message.value(QStringLiteral("file_size")).toDouble());
    const QString sha256 = message.value(QStringLiteral("sha256")).toString();
    const qint64 requestedChunkSize =
        static_cast<qint64>(message.value(QStringLiteral("chunk_size")).toDouble(kTransferChunkSize));
    const quint32 chunkSize = requestedChunkSize > 0
        ? static_cast<quint32>(qMin<qint64>(requestedChunkSize, kTransferChunkSize))
        : kTransferChunkSize;
    const bool isFolder = message.value(QStringLiteral("is_folder")).toBool(false);

    if (transferId.isEmpty() || fromPeerId.isEmpty() || fileName.isEmpty()) {
        qWarning() << "Invalid transfer.request: missing required fields";
        return;
    }

    if (isFolder) {
        const QJsonObject error = Protocol::buildTransferReject(
            transferId,
            QStringLiteral("文件夹传输暂未实现"));
        connection->sendMessage(error);
        return;
    }

    QString downloadDir = m_settings ? m_settings->downloadDir() : QString();
    if (downloadDir.trimmed().isEmpty()) {
        downloadDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    }
    if (downloadDir.trimmed().isEmpty()) {
        downloadDir = QDir::currentPath();
    }
    const QString safeFileName = sanitizedFileName(fileName);
    const QString savePath = uniqueFilePath(downloadDir, safeFileName);

    // 构建传输上下文（接收方）
    TransferContext ctx;
    ctx.task.transferId = transferId;
    ctx.task.direction = TransferDirection::Download;
    ctx.task.peerId = fromPeerId;
    ctx.task.fileName = safeFileName;
    ctx.task.filePath = savePath;
    ctx.task.fileSize = fileSize;
    ctx.task.sha256 = sha256;
    ctx.task.status = TransferStatus::Waiting;
    ctx.task.createdAt = QDateTime::currentDateTimeUtc();
    ctx.chunkSize = chunkSize;
    ctx.isOutgoing = false;

    // 记录与该 transferId 关联的 peer（用于后续查找连接）
    ctx.peer.peerId = fromPeerId;

    // 持久化到 SQLite
    if (m_transferRepo) {
        m_transferRepo->saveTask(ctx.task);
    }

    // 注册到活跃传输映射
    m_activeTransfers.insert(transferId, ctx);

    qInfo() << "Received transfer request:" << transferId << fileName << fileSize << "bytes";

    // 通知 UI 层弹出接受/拒绝对话框
    emit transferRequested(ctx.task);
}

void CourierService::handleTransferAccept(const QJsonObject& message)
{
    const QString transferId = message.value(QStringLiteral("transfer_id")).toString();

    auto it = m_activeTransfers.find(transferId);
    if (it == m_activeTransfers.end()) {
        qWarning() << "handleTransferAccept: unknown transfer" << transferId;
        return;
    }

    TransferContext& ctx = it.value();
    if (!ctx.isOutgoing) {
        qWarning() << "handleTransferAccept: transfer" << transferId << "is not outgoing";
        return;
    }

    // 更新状态为传输中
    ctx.task.status = TransferStatus::Transferring;
    if (m_transferRepo) {
        m_transferRepo->updateStatus(transferId, TransferStatus::Transferring);
    }

    emit transferAccepted(transferId);

    qInfo() << "Transfer accepted by peer:" << transferId;

    // 开始分块发送
    startSendingChunks(transferId);
}

void CourierService::handleTransferReject(const QJsonObject& message)
{
    const QString transferId = message.value(QStringLiteral("transfer_id")).toString();
    const QString reason = message.value(QStringLiteral("reason")).toString();

    const QString rejectMessage = reason.isEmpty()
        ? QStringLiteral("对方已拒绝")
        : QStringLiteral("对方已拒绝: %1").arg(reason);
    finishTransfer(transferId, false, rejectMessage);
    emit transferRejected(transferId, reason);
}

void CourierService::handleTransferComplete(const QJsonObject& message)
{
    const QString transferId = message.value(QStringLiteral("transfer_id")).toString();
    const QString remoteSha256 = message.value(QStringLiteral("sha256")).toString();

    auto it = m_activeTransfers.find(transferId);
    if (it == m_activeTransfers.end()) {
        qWarning() << "handleTransferComplete: unknown transfer" << transferId;
        return;
    }

    TransferContext& ctx = it.value();
    if (ctx.isOutgoing) {
        qWarning() << "handleTransferComplete: transfer" << transferId << "is outgoing";
        return;
    }

    if (!ctx.file || !ctx.file->isOpen()) {
        finishTransfer(transferId, false, QStringLiteral("接收文件尚未打开"));
        return;
    }

    // 完成文件写入
    ctx.file->close();

    // 计算本地 SHA-256 并校验
    QString localSha256;
    if (ctx.hasher) {
        localSha256 = QString::fromLatin1(ctx.hasher->result().toHex());
    }

    bool success = true;
    QString errorMsg;

    if (!remoteSha256.isEmpty() && !localSha256.isEmpty()
        && remoteSha256.compare(localSha256, Qt::CaseInsensitive) != 0) {
        // SHA-256 不匹配：传输完成但文件可能损坏
        qWarning() << "SHA-256 mismatch for transfer" << transferId
                   << "local:" << localSha256 << "remote:" << remoteSha256;
        success = false;
        errorMsg = QStringLiteral("SHA-256 校验失败");
    }

    if (success && !localSha256.isEmpty()) {
        ctx.task.sha256 = localSha256;
    }
    finishTransfer(transferId, success, errorMsg);
}

void CourierService::handleTransferError(const QJsonObject& message)
{
    const QString transferId = message.value(QStringLiteral("transfer_id")).toString();
    const QString errorMessage = message.value(QStringLiteral("error_message")).toString();

    finishTransfer(transferId, false, errorMessage);
}

// ---- Chunk Sending ----

void CourierService::startSendingChunks(const QString& transferId)
{
    // 通过 QTimer 让出事件循环，避免大文件传输阻塞 UI
    QTimer::singleShot(kChunkSendYieldMs, this, [this, transferId]() {
        sendNextChunk(transferId);
    });
}

void CourierService::sendNextChunk(const QString& transferId)
{
    auto it = m_activeTransfers.find(transferId);
    if (it == m_activeTransfers.end()) {
        return;
    }

    TransferContext& ctx = it.value();
    if (!ctx.file || !ctx.file->isOpen()) {
        finishTransfer(transferId, false,
                       QStringLiteral("文件句柄无效"));
        return;
    }

    // 计算当前块的文件偏移和大小
    const qint64 offset = static_cast<qint64>(ctx.nextChunkIndex) * ctx.chunkSize;
    ctx.file->seek(offset);

    // 读取一块数据
    QByteArray chunkData;
    chunkData.resize(static_cast<int>(ctx.chunkSize));
    const qint64 bytesRead = ctx.file->read(chunkData.data(), ctx.chunkSize);
    if (bytesRead <= 0) {
        // 文件读取完毕或出错
        if (bytesRead < 0) {
            finishTransfer(transferId, false,
                           QStringLiteral("读取文件失败: %1").arg(ctx.file->errorString()));
            return;
        }
        // bytesRead == 0：所有块发送完毕
        ctx.file->close();

        // 发送 transfer.complete（含源文件 SHA-256）
        TcpConnection* conn = findConnectionForTransfer(transferId);
        if (conn && conn->isConnected()) {
            const QJsonObject complete = Protocol::buildTransferComplete(
                transferId, ctx.task.sha256);
            conn->sendMessage(complete);
        }

        finishTransfer(transferId, true);
        return;
    }

    chunkData.resize(static_cast<int>(bytesRead));

    // 构建并发送 chunk 帧
    TcpConnection* conn = findConnectionForTransfer(transferId);
    if (!conn || !conn->isConnected()) {
        finishTransfer(transferId, false,
                       QStringLiteral("对端连接已断开"));
        return;
    }

    const QByteArray chunkFrame = Protocol::buildChunkFrame(
        transferId, ctx.nextChunkIndex, chunkData);

    if (!conn->sendBinaryChunk(chunkFrame)) {
        finishTransfer(transferId, false,
                       QStringLiteral("发送数据块失败"));
        return;
    }

    // 更新进度
    ctx.task.transferredBytes += bytesRead;
    ctx.nextChunkIndex++;

    if (m_transferRepo) {
        m_transferRepo->updateProgress(transferId, ctx.task.transferredBytes);
    }

    emit transferProgress(transferId, ctx.task.transferredBytes, ctx.task.fileSize);

    // 让出事件循环后发送下一块
    QTimer::singleShot(kChunkSendYieldMs, this, [this, transferId]() {
        sendNextChunk(transferId);
    });
}

// ---- Helpers ----

void CourierService::finishTransfer(const QString& transferId, bool success,
                                    const QString& errorMessage)
{
    auto it = m_activeTransfers.find(transferId);
    if (it == m_activeTransfers.end()) {
        return;
    }

    TransferContext& ctx = it.value();

    // 关闭文件
    if (ctx.file) {
        if (ctx.file->isOpen()) {
            ctx.file->close();
        }
        delete ctx.file;
        ctx.file = nullptr;
    }

    // 更新持久化状态
    if (m_transferRepo) {
        if (success) {
            const QString sha256 = ctx.task.sha256;
            m_transferRepo->markCompleted(transferId,
                                          sha256);
        } else {
            TransferStatus failStatus = TransferStatus::Failed;
            if (errorMessage.contains(QStringLiteral("取消"))
                || errorMessage.contains(QStringLiteral("CANCELLED"))) {
                failStatus = TransferStatus::Cancelled;
            } else if (errorMessage.contains(QStringLiteral("拒绝"))
                       || errorMessage.contains(QStringLiteral("REJECTED"))) {
                failStatus = TransferStatus::Rejected;
            }
            m_transferRepo->updateStatus(transferId, failStatus, errorMessage);
        }
    }

    // 释放 SHA-256 累加器。必须在持久化之后，接收方 hash 已在 complete 处理中写回 task。
    if (ctx.hasher) {
        delete ctx.hasher;
        ctx.hasher = nullptr;
    }

    // 发射信号
    if (success) {
        qInfo() << "Transfer completed:" << transferId;
        emit transferCompleted(transferId);
    } else {
        qWarning() << "Transfer failed:" << transferId << errorMessage;
        emit transferFailed(transferId, errorMessage);
    }

    // 从活跃映射中移除
    m_activeTransfers.erase(it);
}

QString CourierService::generateTransferId() const
{
    return QStringLiteral("tr_%1").arg(
        QUuid::createUuid().toString(QUuid::Id128));
}

QString CourierService::computeFileSha256(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open file for SHA-256 computation:" << filePath;
        return QString();
    }

    QCryptographicHash hasher(QCryptographicHash::Sha256);
    QByteArray buffer;
    buffer.resize(kHashBufferSize);

    while (!file.atEnd()) {
        const qint64 bytesRead = file.read(buffer.data(), kHashBufferSize);
        if (bytesRead > 0) {
            hasher.addData(QByteArrayView(buffer.constData(),
                                          static_cast<int>(bytesRead)));
        }
    }

    file.close();
    return QString::fromLatin1(hasher.result().toHex());
}

TcpConnection* CourierService::findConnectionForTransfer(const QString& transferId) const
{
    auto it = m_activeTransfers.find(transferId);
    if (it == m_activeTransfers.end()) {
        return nullptr;
    }

    // 通过 SignalService 查找对等体的连接
    if (m_signalService) {
        return m_signalService->connectionTo(it->peer.peerId);
    }

    return nullptr;
}

void CourierService::sendTransferError(const QString& transferId,
                                       const QString& errorCode,
                                       const QString& errorMessage)
{
    TcpConnection* conn = findConnectionForTransfer(transferId);
    if (!conn || !conn->isConnected()) {
        return;
    }

    const QJsonObject error = Protocol::buildTransferError(
        transferId,
        errorCode,
        errorMessage);
    conn->sendMessage(error);
}

// ---- 查询 ----

QList<TransferTask> CourierService::allTasks() const
{
    if (!m_transferRepo) {
        return {};
    }
    return m_transferRepo->getAllTasks();
}

QList<TransferTask> CourierService::activeTasks() const
{
    if (!m_transferRepo) {
        return {};
    }
    return m_transferRepo->getActiveTasks();
}

} // namespace FengSui
