// ShareRepository.h
// SQLite shared_folders 表访问封装。

#pragma once

#include "models/SharedFolder.h"

#include <QList>
#include <QObject>
#include <QString>
#include <optional>

class QSqlQuery;

namespace FengSui {

class Database;

// 共享目录持久化仓库。
// 仅访问 shared_folders 表，不处理路径合法性或业务状态广播。
class ShareRepository : public QObject {
    Q_OBJECT

public:
    explicit ShareRepository(Database* database, QObject* parent = nullptr);

    bool saveSharedFolder(const SharedFolder& folder);
    QList<SharedFolder> getAllSharedFolders() const;
    QList<SharedFolder> getActiveSharedFolders() const;
    std::optional<SharedFolder> getSharedFolder(const QString& shareId) const;
    bool setActive(const QString& shareId, bool active);
    bool removeSharedFolder(const QString& shareId);

private:
    static SharedFolder folderFromQuery(const QSqlQuery& query);

    Database* m_database = nullptr;
};

} // namespace FengSui
