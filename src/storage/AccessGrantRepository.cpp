// AccessGrantRepository.cpp
// SQLite access_grants 表访问实现。

#include "storage/AccessGrantRepository.h"

#include "storage/Database.h"

#include <QDateTime>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace FengSui {

AccessGrantRepository::AccessGrantRepository(Database* database, QObject* parent)
    : QObject(parent)
    , m_database(database)
{
}

bool AccessGrantRepository::saveGrant(const AccessGrant& grant)
{
    if (!m_database) {
        qWarning() << "AccessGrantRepository has no database";
        return false;
    }
    if (grant.peerId.trimmed().isEmpty() || grant.shareId.trimmed().isEmpty()) {
        qWarning() << "Cannot save incomplete access grant";
        return false;
    }

    const QDateTime grantedAt = grant.grantedAt.isValid()
        ? grant.grantedAt.toUTC()
        : QDateTime::currentDateTimeUtc();

    QSqlQuery query(m_database->database());
    query.prepare("INSERT OR REPLACE INTO access_grants "
                  "(peer_id, share_id, granted_at, remember) "
                  "VALUES (?, ?, ?, ?)");
    query.addBindValue(grant.peerId.trimmed());
    query.addBindValue(grant.shareId.trimmed());
    query.addBindValue(grantedAt.toString(Qt::ISODate));
    query.addBindValue(grant.remember ? 1 : 0);

    if (!query.exec()) {
        qWarning() << "Failed to save access grant:" << query.lastError().text();
        return false;
    }
    return true;
}

std::optional<AccessGrant> AccessGrantRepository::grantFor(
    const QString& peerId,
    const QString& shareId) const
{
    if (!m_database || peerId.trimmed().isEmpty() || shareId.trimmed().isEmpty()) {
        return std::nullopt;
    }

    QSqlQuery query(m_database->database());
    query.prepare("SELECT peer_id, share_id, granted_at, remember "
                  "FROM access_grants WHERE peer_id = ? AND share_id = ?");
    query.addBindValue(peerId.trimmed());
    query.addBindValue(shareId.trimmed());

    if (!query.exec()) {
        qWarning() << "Failed to query access grant:" << query.lastError().text();
        return std::nullopt;
    }
    if (!query.next()) {
        return std::nullopt;
    }
    return grantFromQuery(query);
}

bool AccessGrantRepository::hasGrant(const QString& peerId,
                                     const QString& shareId) const
{
    return grantFor(peerId, shareId).has_value();
}

AccessGrant AccessGrantRepository::grantFromQuery(const QSqlQuery& query)
{
    AccessGrant grant;
    grant.peerId = query.value(0).toString();
    grant.shareId = query.value(1).toString();
    grant.grantedAt = QDateTime::fromString(query.value(2).toString(), Qt::ISODate);
    grant.remember = query.value(3).toInt() != 0;
    return grant;
}

} // namespace FengSui
