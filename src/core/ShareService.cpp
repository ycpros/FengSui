// ShareService.cpp
// 本机共享目录管理服务实现。

#include "core/ShareService.h"

#include "storage/ShareRepository.h"

#include <QDir>
#include <QFileInfo>
#include <QUuid>

namespace FengSui {

ShareService::ShareService(QObject* parent)
    : QObject(parent)
{
}

void ShareService::setShareRepository(ShareRepository* repository)
{
    m_repository = repository;
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

} // namespace FengSui
