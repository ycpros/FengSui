// ShareService.cpp
// 共享目录服务实现：本机共享管理、远程浏览下载和访问授权。

#include "core/ShareService.h"

#include "app/AppSettings.h"
#include "network/Protocol.h"
#include "network/TcpConnection.h"
#include "storage/AccessGrantRepository.h"
#include "storage/DownloadLogRepository.h"
#include "storage/ShareRepository.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTimer>
#include <QUuid>

namespace FengSui {

namespace {

// 共享下载复用 transfer.chunk 的二进制帧格式，单块上限保持 8 MB。
constexpr quint32 kShareDownloadChunkSize = 8 * 1024 * 1024;

// SHA-256 计算缓冲大小，避免一次性读入大文件。
constexpr int kHashBufferSize = 1024 * 1024;

QString normalizeRemotePath(const QString& path)
{
    QString normalized = path.trimmed();
    normalized.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (normalized.isEmpty()) {
        normalized = QStringLiteral("/");
    }
    if (!normalized.startsWith(QLatin1Char('/'))) {
        normalized.prepend(QLatin1Char('/'));
    }
    return QDir::cleanPath(normalized);
}

bool containsParentSegment(const QString& path)
{
    const QString normalized = normalizeRemotePath(path);
    const QStringList parts = normalized.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    for (const QString& part : parts) {
        if (part == QStringLiteral("..")) {
            return true;
        }
    }
    return false;
}

QString childRemotePath(const QString& parentPath, const QString& name)
{
    const QString parent = normalizeRemotePath(parentPath);
    if (parent == QStringLiteral("/")) {
        return QStringLiteral("/%1").arg(name);
    }
    return QStringLiteral("%1/%2").arg(parent, name);
}

QString sanitizedFileName(const QString& fileName)
{
    QString normalized = fileName;
    normalized.replace(QLatin1Char('\\'), QLatin1Char('/'));
    QString safeName = QFileInfo(normalized).fileName().trimmed();
    if (safeName.isEmpty() || safeName == QStringLiteral(".")
        || safeName == QStringLiteral("..")) {
        safeName = QStringLiteral("fengsui-share-download");
    }
    return safeName;
}

int recursiveFileCount(const QString& directoryPath)
{
    int count = 0;
    QDirIterator iterator(directoryPath,
                          QDir::Files | QDir::NoDotAndDotDot,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        ++count;
    }
    return count;
}

} // namespace

ShareService::ShareService(QObject* parent)
    : QObject(parent)
{
}

ShareService::~ShareService()
{
    const QList<QString> ids = m_downloads.keys();
    for (const QString& id : ids) {
        finishDownload(id, false, QStringLiteral("共享服务关闭"));
    }
}

void ShareService::setShareRepository(ShareRepository* repository)
{
    m_repository = repository;
}

void ShareService::setSignalService(SignalService* service)
{
    m_signalService = service;
}

void ShareService::setAppSettings(AppSettings* settings)
{
    m_settings = settings;
}

void ShareService::setDownloadLogRepository(DownloadLogRepository* repository)
{
    m_downloadLogRepository = repository;
}

void ShareService::setAccessGrantRepository(AccessGrantRepository* repository)
{
    m_accessGrantRepository = repository;
}

std::optional<SharedFolder> ShareService::addSharedFolder(
    const QString& localPath,
    const QString& displayName)
{
    if (!m_repository) {
        return std::nullopt;
    }

    const QString normalizedPath = normalizePath(localPath);
    if (normalizedPath.isEmpty()) {
        return std::nullopt;
    }

    const QFileInfo info(normalizedPath);
    if (!info.exists() || !info.isDir()) {
        return std::nullopt;
    }

    const bool previousAvailability = hasActiveShares();

    std::optional<SharedFolder> existing = findByPath(normalizedPath);
    if (existing.has_value()) {
        SharedFolder folder = existing.value();
        folder.isActive = true;
        if (!displayName.trimmed().isEmpty()) {
            folder.displayName = displayName.trimmed();
        }
        if (!m_repository->saveSharedFolder(folder)) {
            return std::nullopt;
        }
        emitChangedIfNeeded(previousAvailability);
        return folder;
    }

    SharedFolder folder;
    folder.shareId = QStringLiteral("sh_%1").arg(
        QUuid::createUuid().toString(QUuid::Id128));
    folder.localPath = normalizedPath;
    folder.displayName = displayName.trimmed().isEmpty()
        ? defaultDisplayName(normalizedPath)
        : displayName.trimmed();
    folder.isActive = true;

    if (!m_repository->saveSharedFolder(folder)) {
        return std::nullopt;
    }

    emitChangedIfNeeded(previousAvailability);
    return folder;
}

bool ShareService::removeSharedFolder(const QString& shareId)
{
    if (!m_repository) {
        return false;
    }

    const bool previousAvailability = hasActiveShares();
    if (!m_repository->removeSharedFolder(shareId)) {
        return false;
    }

    emitChangedIfNeeded(previousAvailability);
    return true;
}

bool ShareService::setSharedFolderActive(const QString& shareId, bool active)
{
    if (!m_repository) {
        return false;
    }

    const bool previousAvailability = hasActiveShares();
    if (!m_repository->setActive(shareId, active)) {
        return false;
    }

    emitChangedIfNeeded(previousAvailability);
    return true;
}

QList<SharedFolder> ShareService::sharedFolders() const
{
    return m_repository ? m_repository->getAllSharedFolders() : QList<SharedFolder>();
}

QList<SharedFolder> ShareService::activeSharedFolders() const
{
    return m_repository ? m_repository->getActiveSharedFolders() : QList<SharedFolder>();
}

bool ShareService::hasActiveShares() const
{
    return !activeSharedFolders().isEmpty();
}

QString ShareService::requestShareList(const PeerInfo& peer)
{
    const QString requestId = generateRequestId(QStringLiteral("sr"));
    if (peer.peerId.trimmed().isEmpty()) {
        emit remoteShareFailed(requestId, QStringLiteral("共享服务尚未准备好"));
        return requestId;
    }

    const QJsonObject request = Protocol::buildShareListRequest(
        requestId, localPeerId(), peer.peerId);
    emit outboundShareMessage(peer, request);
    return requestId;
}

QString ShareService::requestShareItems(const PeerInfo& peer,
                                        const QString& shareId,
                                        const QString& path)
{
    const QString requestId = generateRequestId(QStringLiteral("sr"));
    if (peer.peerId.trimmed().isEmpty()
        || shareId.trimmed().isEmpty()) {
        emit remoteShareFailed(requestId, QStringLiteral("共享目录请求无效"));
        return requestId;
    }

    const QJsonObject request = Protocol::buildShareItemsRequest(
        requestId, localPeerId(), peer.peerId, shareId.trimmed(),
        normalizeRemotePath(path));
    emit outboundShareMessage(peer, request);
    return requestId;
}

QString ShareService::requestShareDownload(const PeerInfo& peer,
                                           const QString& shareId,
                                           const QString& path)
{
    const QString downloadId = generateRequestId(QStringLiteral("sd"));
    if (peer.peerId.trimmed().isEmpty()
        || shareId.trimmed().isEmpty()) {
        emit remoteShareFailed(downloadId, QStringLiteral("共享下载请求无效"));
        return downloadId;
    }

    const QJsonObject request = Protocol::buildShareDownloadRequest(
        downloadId, localPeerId(), peer.peerId, shareId.trimmed(),
        normalizeRemotePath(path));
    emit outboundShareMessage(peer, request);
    return downloadId;
}

void ShareService::handleShareMessage(TcpConnection* connection,
                                      const QJsonObject& message)
{
    const QString type = Protocol::messageType(message);
    if (type == QStringLiteral("share.list")) {
        handleShareListRequest(connection, message);
    } else if (type == QStringLiteral("share.items")) {
        handleShareItemsRequest(connection, message);
    } else if (type == QStringLiteral("share.download")) {
        handleShareDownloadRequest(connection, message);
    } else if (type == QStringLiteral("share.list.reply")) {
        handleShareListReply(message);
    } else if (type == QStringLiteral("share.items.reply")) {
        handleShareItemsReply(message);
    } else if (type == QStringLiteral("share.download.reply")) {
        handleShareDownloadReply(connection, message);
    } else if (type == QStringLiteral("share.download.complete")) {
        handleShareDownloadComplete(message);
    } else if (type == QStringLiteral("share.error")) {
        handleShareError(message);
    }
}

void ShareService::handleDownloadChunk(const QString& downloadId,
                                       quint32 chunkIndex,
                                       const QByteArray& data)
{
    auto it = m_downloads.find(downloadId);
    if (it == m_downloads.end() || it.value().outgoing) {
        return;
    }

    DownloadContext& context = it.value();
    if (!context.file || !context.file->isOpen()) {
        finishDownload(downloadId, false, QStringLiteral("下载文件尚未准备好"));
        return;
    }
    if (chunkIndex != context.nextChunkIndex) {
        finishDownload(downloadId, false,
                       QStringLiteral("下载数据块顺序错误"));
        return;
    }

    const qint64 bytesWritten = context.file->write(data);
    if (bytesWritten != data.size()) {
        finishDownload(downloadId, false,
                       QStringLiteral("写入下载文件失败: %1")
                           .arg(context.file->errorString()));
        return;
    }

    if (context.hasher) {
        context.hasher->addData(data);
    }
    context.transferredBytes += bytesWritten;
    context.nextChunkIndex++;
    emit remoteShareDownloadProgress(downloadId,
                                     context.transferredBytes,
                                     context.fileSize);
}

void ShareService::resolveAccessRequest(const QString& requestId,
                                        bool allowed,
                                        bool remember)
{
    auto it = m_pendingAccessRequests.find(requestId);
    if (it == m_pendingAccessRequests.end()) {
        return;
    }

    const PendingAccessRequest request = it.value();
    m_pendingAccessRequests.erase(it);

    if (!allowed) {
        if (request.connection) {
            sendShareError(request.connection,
                           request.requestId,
                           request.peerId,
                           QStringLiteral("ACCESS_DENIED"),
                           QStringLiteral("访问被拒绝"));
        }
        return;
    }

    for (const QString& shareId : request.shareIds) {
        m_sessionAccessGrants.insert(accessKey(request.peerId, shareId));
        if (remember && m_accessGrantRepository) {
            AccessGrant grant;
            grant.peerId = request.peerId;
            grant.shareId = shareId;
            grant.grantedAt = QDateTime::currentDateTimeUtc();
            grant.remember = true;
            m_accessGrantRepository->saveGrant(grant);
        }
    }

    if (!request.connection) {
        return;
    }
    continueAuthorizedRequest(request);
}

QString ShareService::normalizePath(const QString& localPath)
{
    const QString trimmed = localPath.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }

    const QFileInfo info(trimmed);
    const QString absolutePath = info.absoluteFilePath();
    return QDir::cleanPath(absolutePath);
}

QString ShareService::defaultDisplayName(const QString& normalizedPath)
{
    const QFileInfo info(normalizedPath);
    const QString name = info.fileName().trimmed();
    return name.isEmpty() ? normalizedPath : name;
}

std::optional<SharedFolder> ShareService::findByPath(
    const QString& normalizedPath) const
{
    if (!m_repository) {
        return std::nullopt;
    }

    const QString cleanTarget = QDir::cleanPath(normalizedPath);
    const QList<SharedFolder> folders = m_repository->getAllSharedFolders();
    for (const SharedFolder& folder : folders) {
        if (QDir::cleanPath(folder.localPath) == cleanTarget) {
            return folder;
        }
    }
    return std::nullopt;
}

void ShareService::emitChangedIfNeeded(bool previousAvailability)
{
    emit sharedFoldersChanged();

    const bool currentAvailability = hasActiveShares();
    if (previousAvailability != currentAvailability) {
        emit shareAvailabilityChanged(currentAvailability);
    }
}

QString ShareService::localPeerId() const
{
    return m_settings ? m_settings->peerId().trimmed() : QString();
}

QString ShareService::generateRequestId(const QString& prefix) const
{
    return QStringLiteral("%1_%2").arg(prefix, QUuid::createUuid().toString(QUuid::Id128));
}

QString ShareService::accessKey(const QString& peerId, const QString& shareId) const
{
    return QStringLiteral("%1|%2").arg(peerId.trimmed(), shareId.trimmed());
}

bool ShareService::hasAccess(const QString& peerId, const QString& shareId) const
{
    if (peerId.trimmed().isEmpty() || shareId.trimmed().isEmpty()) {
        return false;
    }
    if (m_sessionAccessGrants.contains(accessKey(peerId, shareId))) {
        return true;
    }
    return m_accessGrantRepository
        && m_accessGrantRepository->hasGrant(peerId.trimmed(), shareId.trimmed());
}

bool ShareService::ensureAccessOrDefer(TcpConnection* connection,
                                       const QJsonObject& message,
                                       const QStringList& shareIds,
                                       const QString& shareName)
{
    const QString peerId = message.value(QStringLiteral("from")).toString().trimmed();
    if (peerId.isEmpty()) {
        sendShareError(connection,
                       message.value(QStringLiteral("request_id"))
                           .toString(message.value(QStringLiteral("download_id")).toString()),
                       peerId,
                       QStringLiteral("INVALID_MESSAGE"),
                       QStringLiteral("共享请求缺少来源设备"));
        return false;
    }

    QStringList missingShareIds;
    for (const QString& shareId : shareIds) {
        if (!hasAccess(peerId, shareId)) {
            missingShareIds.append(shareId);
        }
    }
    if (missingShareIds.isEmpty()) {
        return true;
    }

    const QString requestId = message.value(QStringLiteral("request_id"))
                                  .toString(message.value(QStringLiteral("download_id"))
                                                .toString());
    PendingAccessRequest request;
    request.requestId = requestId;
    request.peerId = peerId;
    request.requesterName = message.value(QStringLiteral("display_name"))
                                .toString(peerId);
    request.deviceName = message.value(QStringLiteral("device_name"))
                             .toString(QStringLiteral("未命名设备"));
    request.shareName = shareName;
    request.shareIds = missingShareIds;
    request.message = message;
    request.connection = connection;
    m_pendingAccessRequests.insert(requestId, request);

    emit accessRequested(requestId,
                         request.requesterName,
                         request.deviceName,
                         shareName);
    return false;
}

void ShareService::continueAuthorizedRequest(const PendingAccessRequest& request)
{
    if (!request.connection) {
        return;
    }
    handleShareMessage(request.connection, request.message);
}

void ShareService::sendShareError(TcpConnection* connection,
                                  const QString& requestId,
                                  const QString& toPeerId,
                                  const QString& errorCode,
                                  const QString& errorMessage)
{
    if (!connection || !connection->isConnected()) {
        return;
    }
    const QJsonObject error = Protocol::buildShareError(
        requestId, localPeerId(), toPeerId, errorCode, errorMessage);
    connection->sendMessage(error);
}

void ShareService::handleShareListRequest(TcpConnection* connection,
                                          const QJsonObject& message)
{
    const QString requestId = message.value(QStringLiteral("request_id")).toString();
    const QString fromPeerId = message.value(QStringLiteral("from")).toString();
    const QList<SharedFolder> folders = activeSharedFolders();
    QStringList shareIds;
    for (const SharedFolder& folder : folders) {
        shareIds.append(folder.shareId);
    }
    if (!ensureAccessOrDefer(connection, message, shareIds,
                             QStringLiteral("共享目录列表"))) {
        return;
    }

    const QJsonObject reply = Protocol::buildShareListReply(
        requestId, localPeerId(), fromPeerId, buildLocalShareList());
    if (connection) {
        connection->sendMessage(reply);
    }
}

void ShareService::handleShareItemsRequest(TcpConnection* connection,
                                           const QJsonObject& message)
{
    const QString requestId = message.value(QStringLiteral("request_id")).toString();
    const QString fromPeerId = message.value(QStringLiteral("from")).toString();
    const QString shareId = message.value(QStringLiteral("share_id")).toString().trimmed();
    const QString remotePath = normalizeRemotePath(message.value(QStringLiteral("path")).toString());
    const std::optional<SharedFolder> folder = activeShareById(shareId);
    if (!folder.has_value()) {
        sendShareError(connection, requestId, fromPeerId,
                       QStringLiteral("FILE_NOT_FOUND"),
                       QStringLiteral("共享目录不存在或已停用"));
        return;
    }
    if (!ensureAccessOrDefer(connection, message, {shareId}, folder->displayName)) {
        return;
    }

    QString error;
    const QJsonArray items = buildLocalItems(folder.value(), remotePath, error);
    if (!error.isEmpty()) {
        sendShareError(connection, requestId, fromPeerId,
                       QStringLiteral("FILE_NOT_FOUND"), error);
        return;
    }

    const QJsonObject reply = Protocol::buildShareItemsReply(
        requestId, localPeerId(), fromPeerId, shareId, remotePath, items);
    if (connection) {
        connection->sendMessage(reply);
    }
}

void ShareService::handleShareDownloadRequest(TcpConnection* connection,
                                              const QJsonObject& message)
{
    const QString downloadId = message.value(QStringLiteral("download_id")).toString();
    const QString fromPeerId = message.value(QStringLiteral("from")).toString();
    const QString shareId = message.value(QStringLiteral("share_id")).toString().trimmed();
    const QString remotePath = normalizeRemotePath(message.value(QStringLiteral("path")).toString());
    const std::optional<SharedFolder> folder = activeShareById(shareId);
    if (!folder.has_value()) {
        sendShareError(connection, downloadId, fromPeerId,
                       QStringLiteral("FILE_NOT_FOUND"),
                       QStringLiteral("共享目录不存在或已停用"));
        return;
    }
    if (!ensureAccessOrDefer(connection, message, {shareId}, folder->displayName)) {
        return;
    }

    QString error;
    const QString filePath = safeLocalPath(folder.value(), remotePath, true, error);
    if (!error.isEmpty()) {
        sendShareError(connection, downloadId, fromPeerId,
                       QStringLiteral("FILE_NOT_FOUND"), error);
        return;
    }

    QFileInfo info(filePath);
    DownloadContext context;
    context.downloadId = downloadId;
    context.peer.peerId = fromPeerId;
    context.shareId = shareId;
    context.remotePath = remotePath;
    context.localPath = filePath;
    context.fileName = info.fileName();
    context.fileSize = info.size();
    context.sha256 = computeFileSha256(filePath);
    context.outgoing = true;
    context.connection = connection;
    context.file = new QFile(filePath);
    if (!context.file->open(QIODevice::ReadOnly)) {
        const QString openError = QStringLiteral("无法读取共享文件: %1")
                                      .arg(context.file->errorString());
        delete context.file;
        context.file = nullptr;
        sendShareError(connection, downloadId, fromPeerId,
                       QStringLiteral("FILE_NOT_FOUND"), openError);
        return;
    }

    m_downloads.insert(downloadId, context);
    const QJsonObject reply = Protocol::buildShareDownloadReply(
        downloadId, localPeerId(), fromPeerId, shareId, remotePath,
        info.fileName(), info.size(), context.sha256);
    if (connection && connection->sendMessage(reply)) {
        startOutgoingDownload(downloadId);
    } else {
        finishDownload(downloadId, false, QStringLiteral("无法发送下载响应"));
    }
}

void ShareService::handleShareListReply(const QJsonObject& message)
{
    const QString peerId = message.value(QStringLiteral("from")).toString();
    const QString requestId = message.value(QStringLiteral("request_id")).toString();
    emit remoteShareListReceived(peerId, requestId,
                                 parseRemoteShares(peerId, message));
}

void ShareService::handleShareItemsReply(const QJsonObject& message)
{
    const QString peerId = message.value(QStringLiteral("from")).toString();
    const QString requestId = message.value(QStringLiteral("request_id")).toString();
    const QString shareId = message.value(QStringLiteral("share_id")).toString();
    const QString path = normalizeRemotePath(message.value(QStringLiteral("path")).toString());
    emit remoteShareItemsReceived(peerId, requestId, shareId, path,
                                  parseRemoteItems(peerId, shareId, message));
}

void ShareService::handleShareDownloadReply(TcpConnection* connection,
                                            const QJsonObject& message)
{
    const QString downloadId = message.value(QStringLiteral("download_id")).toString();
    const QString fileName = sanitizedFileName(
        message.value(QStringLiteral("file_name")).toString());
    if (downloadId.trimmed().isEmpty()) {
        return;
    }

    DownloadContext context;
    context.downloadId = downloadId;
    context.peer.peerId = message.value(QStringLiteral("from")).toString();
    context.shareId = message.value(QStringLiteral("share_id")).toString();
    context.remotePath = normalizeRemotePath(message.value(QStringLiteral("path")).toString());
    context.localPath = uniqueDownloadPath(fileName);
    context.fileName = fileName;
    context.fileSize = static_cast<qint64>(
        message.value(QStringLiteral("file_size")).toDouble());
    context.sha256 = message.value(QStringLiteral("sha256")).toString();
    context.outgoing = false;
    context.connection = connection;
    context.hasher = new QCryptographicHash(QCryptographicHash::Sha256);
    context.file = new QFile(context.localPath);

    const QFileInfo targetInfo(context.localPath);
    QDir targetDir = targetInfo.absoluteDir();
    if (!targetDir.exists() && !QDir().mkpath(targetDir.absolutePath())) {
        delete context.file;
        delete context.hasher;
        emit remoteShareFailed(downloadId,
                               QStringLiteral("无法创建下载目录"));
        return;
    }
    if (!context.file->open(QIODevice::WriteOnly)) {
        const QString error = context.file->errorString();
        delete context.file;
        delete context.hasher;
        emit remoteShareFailed(downloadId,
                               QStringLiteral("无法保存下载文件: %1").arg(error));
        return;
    }

    m_downloads.insert(downloadId, context);
    emit remoteShareDownloadStarted(downloadId,
                                    context.fileName,
                                    context.fileSize,
                                    context.localPath);
}

void ShareService::handleShareDownloadComplete(const QJsonObject& message)
{
    const QString downloadId = message.value(QStringLiteral("download_id")).toString();
    auto it = m_downloads.find(downloadId);
    if (it == m_downloads.end() || it.value().outgoing) {
        return;
    }

    DownloadContext& context = it.value();
    if (context.file && context.file->isOpen()) {
        context.file->close();
    }

    QString localSha256;
    if (context.hasher) {
        localSha256 = QString::fromLatin1(context.hasher->result().toHex());
    }
    const QString remoteSha256 =
        message.value(QStringLiteral("sha256")).toString(context.sha256);
    if (!remoteSha256.isEmpty()
        && !localSha256.isEmpty()
        && remoteSha256.compare(localSha256, Qt::CaseInsensitive) != 0) {
        finishDownload(downloadId, false, QStringLiteral("SHA-256 校验失败"));
        return;
    }
    finishDownload(downloadId, true);
}

void ShareService::handleShareError(const QJsonObject& message)
{
    const QString requestId = message.value(QStringLiteral("request_id")).toString();
    const QString errorMessage =
        message.value(QStringLiteral("error_message")).toString(
            QStringLiteral("共享请求失败"));
    if (m_downloads.contains(requestId)) {
        finishDownload(requestId, false, errorMessage);
    } else {
        emit remoteShareFailed(requestId, errorMessage);
    }
}

QList<RemoteSharedFolder> ShareService::parseRemoteShares(
    const QString& peerId,
    const QJsonObject& message) const
{
    QList<RemoteSharedFolder> result;
    const QJsonArray shares = message.value(QStringLiteral("shares")).toArray();
    for (const QJsonValue& value : shares) {
        const QJsonObject object = value.toObject();
        RemoteSharedFolder folder;
        folder.peerId = peerId;
        folder.shareId = object.value(QStringLiteral("share_id")).toString();
        folder.displayName = object.value(QStringLiteral("display_name")).toString();
        folder.fileCount = object.value(QStringLiteral("file_count")).toInt();
        if (!folder.shareId.trimmed().isEmpty()) {
            result.append(folder);
        }
    }
    return result;
}

QList<RemoteShareItem> ShareService::parseRemoteItems(
    const QString& peerId,
    const QString& shareId,
    const QJsonObject& message) const
{
    QList<RemoteShareItem> result;
    const QJsonArray items = message.value(QStringLiteral("items")).toArray();
    for (const QJsonValue& value : items) {
        const QJsonObject object = value.toObject();
        RemoteShareItem item;
        item.peerId = peerId;
        item.shareId = shareId;
        item.path = normalizeRemotePath(object.value(QStringLiteral("path")).toString());
        item.name = object.value(QStringLiteral("name")).toString();
        item.size = static_cast<qint64>(object.value(QStringLiteral("size")).toDouble());
        item.modifiedAt = object.value(QStringLiteral("modified_at")).toString();
        item.isDir = object.value(QStringLiteral("is_dir")).toBool();
        if (!item.name.trimmed().isEmpty()) {
            result.append(item);
        }
    }
    return result;
}

QJsonArray ShareService::buildLocalShareList() const
{
    QJsonArray shares;
    for (const SharedFolder& folder : activeSharedFolders()) {
        QJsonObject object;
        object.insert(QStringLiteral("share_id"), folder.shareId);
        object.insert(QStringLiteral("display_name"), folder.displayName);
        object.insert(QStringLiteral("file_count"), recursiveFileCount(folder.localPath));
        shares.append(object);
    }
    return shares;
}

QJsonArray ShareService::buildLocalItems(const SharedFolder& folder,
                                         const QString& remotePath,
                                         QString& errorOut) const
{
    QJsonArray items;
    const QString localPath = safeLocalPath(folder, remotePath, false, errorOut);
    if (!errorOut.isEmpty()) {
        return items;
    }

    const QDir dir(localPath);
    const QFileInfoList entries = dir.entryInfoList(
        QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
        QDir::DirsFirst | QDir::Name);
    for (const QFileInfo& info : entries) {
        QJsonObject object;
        object.insert(QStringLiteral("name"), info.fileName());
        object.insert(QStringLiteral("path"),
                      childRemotePath(remotePath, info.fileName()));
        object.insert(QStringLiteral("is_dir"), info.isDir());
        object.insert(QStringLiteral("size"), info.isDir() ? 0 : info.size());
        object.insert(QStringLiteral("modified_at"),
                      info.lastModified().toUTC().toString(Qt::ISODate));
        items.append(object);
    }
    return items;
}

std::optional<SharedFolder> ShareService::activeShareById(
    const QString& shareId) const
{
    if (!m_repository || shareId.trimmed().isEmpty()) {
        return std::nullopt;
    }
    const std::optional<SharedFolder> folder =
        m_repository->getSharedFolder(shareId.trimmed());
    if (!folder.has_value() || !folder->isActive) {
        return std::nullopt;
    }
    return folder;
}

QString ShareService::safeLocalPath(const SharedFolder& folder,
                                    const QString& remotePath,
                                    bool requireFile,
                                    QString& errorOut) const
{
    if (containsParentSegment(remotePath)) {
        errorOut = QStringLiteral("路径不允许包含上级目录");
        return {};
    }

    const QFileInfo rootInfo(folder.localPath);
    const QString rootPath = QDir(rootInfo.absoluteFilePath()).canonicalPath();
    if (rootPath.isEmpty()) {
        errorOut = QStringLiteral("共享根目录不可访问");
        return {};
    }

    QString relative = normalizeRemotePath(remotePath);
    if (relative == QStringLiteral("/")) {
        relative.clear();
    } else {
        relative.remove(0, 1);
    }

    const QString candidate = QDir(rootPath).filePath(relative);
    const QFileInfo candidateInfo(candidate);
    const QString canonical = candidateInfo.canonicalFilePath();
    if (canonical.isEmpty()) {
        errorOut = QStringLiteral("请求的路径不存在");
        return {};
    }

    const QString cleanRoot = QDir::cleanPath(rootPath);
    const QString cleanCandidate = QDir::cleanPath(canonical);
    if (cleanCandidate != cleanRoot
        && !cleanCandidate.startsWith(cleanRoot + QLatin1Char('/'))) {
        errorOut = QStringLiteral("请求路径超出共享目录");
        return {};
    }
    if (requireFile && !candidateInfo.isFile()) {
        errorOut = QStringLiteral("请求的路径不是文件");
        return {};
    }
    if (!requireFile && !candidateInfo.isDir()) {
        errorOut = QStringLiteral("请求的路径不是目录");
        return {};
    }
    return cleanCandidate;
}

QString ShareService::uniqueDownloadPath(const QString& fileName) const
{
    QString downloadDir = m_settings ? m_settings->downloadDir() : QString();
    if (downloadDir.trimmed().isEmpty()) {
        downloadDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    }
    if (downloadDir.trimmed().isEmpty()) {
        downloadDir = QDir::currentPath();
    }

    QDir dir(downloadDir);
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

void ShareService::startOutgoingDownload(const QString& downloadId)
{
    QTimer::singleShot(0, this, [this, downloadId]() {
        sendNextDownloadChunk(downloadId);
    });
}

void ShareService::sendNextDownloadChunk(const QString& downloadId)
{
    auto it = m_downloads.find(downloadId);
    if (it == m_downloads.end() || !it.value().outgoing) {
        return;
    }

    DownloadContext& context = it.value();
    if (!context.file || !context.file->isOpen()) {
        finishDownload(downloadId, false, QStringLiteral("共享文件句柄无效"));
        return;
    }
    if (!context.connection || !context.connection->isConnected()) {
        finishDownload(downloadId, false, QStringLiteral("对端连接已断开"));
        return;
    }

    QByteArray data;
    data.resize(static_cast<int>(kShareDownloadChunkSize));
    const qint64 bytesRead =
        context.file->read(data.data(), kShareDownloadChunkSize);
    if (bytesRead < 0) {
        finishDownload(downloadId, false,
                       QStringLiteral("读取共享文件失败: %1")
                           .arg(context.file->errorString()));
        return;
    }
    if (bytesRead == 0) {
        context.file->close();
        const QJsonObject complete = Protocol::buildShareDownloadComplete(
            downloadId, localPeerId(), context.peer.peerId, context.sha256);
        context.connection->sendMessage(complete);
        finishDownload(downloadId, true);
        return;
    }

    data.resize(static_cast<int>(bytesRead));
    const QByteArray chunkFrame =
        Protocol::buildChunkFrame(downloadId, context.nextChunkIndex, data);
    if (!context.connection->sendBinaryChunk(chunkFrame)) {
        finishDownload(downloadId, false, QStringLiteral("发送共享数据块失败"));
        return;
    }

    context.transferredBytes += bytesRead;
    context.nextChunkIndex++;
    QTimer::singleShot(0, this, [this, downloadId]() {
        sendNextDownloadChunk(downloadId);
    });
}

void ShareService::finishDownload(const QString& downloadId,
                                  bool success,
                                  const QString& errorMessage)
{
    auto it = m_downloads.find(downloadId);
    if (it == m_downloads.end()) {
        return;
    }

    DownloadContext context = it.value();
    if (context.file) {
        if (context.file->isOpen()) {
            context.file->close();
        }
        delete context.file;
        context.file = nullptr;
    }
    if (context.hasher) {
        delete context.hasher;
        context.hasher = nullptr;
    }

    if (!context.outgoing) {
        saveDownloadLog(context, success);
        if (success) {
            emit remoteShareDownloadCompleted(downloadId, context.localPath);
        } else {
            emit remoteShareFailed(downloadId, errorMessage);
        }
    }

    m_downloads.erase(it);
}

void ShareService::saveDownloadLog(const DownloadContext& context, bool success)
{
    if (!m_downloadLogRepository) {
        return;
    }

    DownloadLog log;
    log.logId = QStringLiteral("dl_%1").arg(QUuid::createUuid().toString(QUuid::Id128));
    log.peerId = context.peer.peerId;
    log.shareId = context.shareId;
    log.remotePath = context.remotePath;
    log.localPath = context.localPath;
    log.fileName = context.fileName;
    log.fileSize = context.fileSize;
    log.downloadedAt = QDateTime::currentDateTimeUtc();
    log.success = success;
    m_downloadLogRepository->saveLog(log);
}

QString ShareService::computeFileSha256(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
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
    return QString::fromLatin1(hasher.result().toHex());
}

} // namespace FengSui
