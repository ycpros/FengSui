// ManualPeerRepository.cpp
// 手动添加设备持久化仓库实现。

#include "storage/ManualPeerRepository.h"

#include "storage/Database.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace FengSui {

ManualPeerRepository::ManualPeerRepository(Database* database, QObject* parent)
    : QObject(parent)
    , m_database(database)
{
}

bool ManualPeerRepository::upsertManualPeer(const ManualPeer& peer)
{
    if (!m_database) {
        return false;
    }
    if (peer.host.trimmed().isEmpty() || peer.port == 0) {
        qWarning() << "Cannot save manual peer with incomplete endpoint";
        return false;
    }

    QSqlQuery query(m_database->database());
    query.prepare("INSERT OR REPLACE INTO manual_peers "
                  "(name, host, port, last_success_at) "
                  "VALUES (?, ?, ?, ?)");
    query.addBindValue(peer.name.trimmed().isEmpty()
                           ? QStringLiteral("%1:%2").arg(peer.host).arg(peer.port)
                           : peer.name.trimmed());
    query.addBindValue(peer.host.trimmed());
    query.addBindValue(peer.port);
    query.addBindValue(peer.lastSuccessAt.toString(Qt::ISODate));

    if (!query.exec()) {
        qWarning() << "Failed to save manual peer:" << query.lastError().text();
        return false;
    }

    return true;
}

QList<ManualPeer> ManualPeerRepository::getAllManualPeers() const
{
    QList<ManualPeer> result;
    if (!m_database) {
        return result;
    }

    QSqlQuery query(m_database->database());
    if (!query.exec("SELECT name, host, port, last_success_at "
                    "FROM manual_peers "
                    "ORDER BY last_success_at DESC")) {
        qWarning() << "Failed to get manual peers:" << query.lastError().text();
        return result;
    }

    while (query.next()) {
        result.append(manualPeerFromQuery(query));
    }
    return result;
}

std::optional<ManualPeer> ManualPeerRepository::getManualPeer(
    const QString& host,
    quint16 port) const
{
    if (!m_database || host.trimmed().isEmpty() || port == 0) {
        return std::nullopt;
    }

    QSqlQuery query(m_database->database());
    query.prepare("SELECT name, host, port, last_success_at "
                  "FROM manual_peers WHERE host = ? AND port = ?");
    query.addBindValue(host.trimmed());
    query.addBindValue(port);

    if (!query.exec()) {
        qWarning() << "Failed to get manual peer:" << query.lastError().text();
        return std::nullopt;
    }
    if (!query.next()) {
        return std::nullopt;
    }

    return manualPeerFromQuery(query);
}

ManualPeer ManualPeerRepository::manualPeerFromQuery(const QSqlQuery& query)
{
    ManualPeer peer;
    peer.name = query.value(0).toString();
    peer.host = query.value(1).toString();
    peer.port = static_cast<quint16>(query.value(2).toUInt());
    peer.lastSuccessAt = QDateTime::fromString(query.value(3).toString(),
                                               Qt::ISODate);
    return peer;
}

} // namespace FengSui
