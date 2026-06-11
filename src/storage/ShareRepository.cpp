// ShareRepository.cpp
// SQLite shared_folders 表访问实现。

#include "storage/ShareRepository.h"

#include "storage/Database.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace FengSui {

ShareRepository::ShareRepository(Database* database, QObject* parent)
    : QObject(parent)
    , m_database(database)
{
}

bool ShareRepository::saveSharedFolder(const SharedFolder& folder)
{
    if (!m_database) {
        qWarning() << "ShareRepository has no database";
        return false;
    }
    if (folder.shareId.trimmed().isEmpty()
        || folder.localPath.trimmed().isEmpty()
        || folder.displayName.trimmed().isEmpty()) {
        qWarning() << "Cannot save incomplete shared folder";
        return false;
    }

    QSqlQuery query(m_database->database());
    query.prepare("INSERT OR REPLACE INTO shared_folders "
                  "(share_id, local_path, display_name, is_active) "
                  "VALUES (?, ?, ?, ?)");
    query.addBindValue(folder.shareId);
    query.addBindValue(folder.localPath);
    query.addBindValue(folder.displayName);
    query.addBindValue(folder.isActive ? 1 : 0);

    if (!query.exec()) {
        qWarning() << "Failed to save shared folder:" << query.lastError().text();
        return false;
    }

    return true;
}

QList<SharedFolder> ShareRepository::getAllSharedFolders() const
{
    QList<SharedFolder> result;
    if (!m_database) {
        return result;
    }

    QSqlQuery query(m_database->database());
    if (!query.exec("SELECT share_id, local_path, display_name, is_active "
                    "FROM shared_folders ORDER BY display_name ASC")) {
        qWarning() << "Failed to get shared folders:" << query.lastError().text();
        return result;
    }

    while (query.next()) {
        result.append(folderFromQuery(query));
    }
    return result;
}

QList<SharedFolder> ShareRepository::getActiveSharedFolders() const
{
    QList<SharedFolder> result;
    if (!m_database) {
        return result;
    }

    QSqlQuery query(m_database->database());
    if (!query.exec("SELECT share_id, local_path, display_name, is_active "
                    "FROM shared_folders WHERE is_active = 1 "
                    "ORDER BY display_name ASC")) {
        qWarning() << "Failed to get active shared folders:" << query.lastError().text();
        return result;
    }

    while (query.next()) {
        result.append(folderFromQuery(query));
    }
    return result;
}

std::optional<SharedFolder> ShareRepository::getSharedFolder(
    const QString& shareId) const
{
    if (!m_database || shareId.trimmed().isEmpty()) {
        return std::nullopt;
    }

    QSqlQuery query(m_database->database());
    query.prepare("SELECT share_id, local_path, display_name, is_active "
                  "FROM shared_folders WHERE share_id = ?");
    query.addBindValue(shareId.trimmed());

    if (!query.exec()) {
        qWarning() << "Failed to get shared folder:" << query.lastError().text();
        return std::nullopt;
    }
    if (!query.next()) {
        return std::nullopt;
    }

    return folderFromQuery(query);
}

bool ShareRepository::setActive(const QString& shareId, bool active)
{
    if (!m_database || shareId.trimmed().isEmpty()) {
        return false;
    }

    QSqlQuery query(m_database->database());
    query.prepare("UPDATE shared_folders SET is_active = ? WHERE share_id = ?");
    query.addBindValue(active ? 1 : 0);
    query.addBindValue(shareId.trimmed());

    if (!query.exec()) {
        qWarning() << "Failed to update shared folder active state:"
                   << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool ShareRepository::removeSharedFolder(const QString& shareId)
{
    if (!m_database || shareId.trimmed().isEmpty()) {
        return false;
    }

    QSqlQuery query(m_database->database());
    query.prepare("DELETE FROM shared_folders WHERE share_id = ?");
    query.addBindValue(shareId.trimmed());

    if (!query.exec()) {
        qWarning() << "Failed to remove shared folder:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

SharedFolder ShareRepository::folderFromQuery(const QSqlQuery& query)
{
    SharedFolder folder;
    folder.shareId = query.value(0).toString();
    folder.localPath = query.value(1).toString();
    folder.displayName = query.value(2).toString();
    folder.isActive = query.value(3).toInt() != 0;
    return folder;
}

} // namespace FengSui
