// ShareService.h
// 本机共享目录管理服务。

#pragma once

#include "models/SharedFolder.h"

#include <QList>
#include <QObject>
#include <QString>
#include <optional>

namespace FengSui {

class ShareRepository;

// 共享目录业务服务。
// 负责本机共享目录的路径校验、去重、启停和状态变更通知。
class ShareService : public QObject {
    Q_OBJECT

public:
    explicit ShareService(QObject* parent = nullptr);

    void setShareRepository(ShareRepository* repository);

    std::optional<SharedFolder> addSharedFolder(
        const QString& localPath,
        const QString& displayName = QString());
    bool removeSharedFolder(const QString& shareId);
    bool setSharedFolderActive(const QString& shareId, bool active);

    QList<SharedFolder> sharedFolders() const;
    QList<SharedFolder> activeSharedFolders() const;
    bool hasActiveShares() const;

signals:
    void sharedFoldersChanged();
    void shareAvailabilityChanged(bool enabled);

private:
    static QString normalizePath(const QString& localPath);
    static QString defaultDisplayName(const QString& normalizedPath);
    std::optional<SharedFolder> findByPath(const QString& normalizedPath) const;
    void emitChangedIfNeeded(bool previousAvailability);

    ShareRepository* m_repository = nullptr;
};

} // namespace FengSui
