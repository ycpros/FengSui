// DownloadLogRepository.cpp
// SQLite download_logs 表访问实现。

#include "storage/DownloadLogRepository.h"

#include "storage/Database.h"

#include <QDateTime>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariant>

namespace FengSui {

DownloadLogRepository::DownloadLogRepository(Database* database, QObject* parent)
    : QObject(parent)
    , m_database(database)
{
}

bool DownloadLogRepository::saveLog(const DownloadLog& log)
{
    if (!m_database) {
        qWarning() << "DownloadLogRepository has no database";
        return false;
    }
    if (log.peerId.trimmed().isEmpty() || log.fileName.trimmed().isEmpty()) {
        qWarning() << "Cannot save incomplete download log";
        return false;
    }

    const QString logId = log.logId.trimmed().isEmpty()
        ? QStringLiteral("dl_%1").arg(QUuid::createUuid().toString(QUuid::Id128))
        : log.logId.trimmed();
    const QDateTime downloadedAt = log.downloadedAt.isValid()
        ? log.downloadedAt.toUTC()
        : QDateTime::currentDateTimeUtc();

    QSqlQuery query(m_database->database());
    query.prepare("INSERT OR REPLACE INTO download_logs "
                  "(log_id, peer_id, share_id, remote_path, local_path, "
                  "file_name, file_size, downloaded_at, success) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(logId);
    query.addBindValue(log.peerId.trimmed());
    query.addBindValue(log.shareId.trimmed());
    query.addBindValue(log.remotePath);
    query.addBindValue(log.localPath);
    query.addBindValue(log.fileName.trimmed());
    query.addBindValue(log.fileSize);
    query.addBindValue(downloadedAt.toString(Qt::ISODate));
    query.addBindValue(log.success ? 1 : 0);

    if (!query.exec()) {
        qWarning() << "Failed to save download log:" << query.lastError().text();
        return false;
    }
    return true;
}

QList<DownloadLog> DownloadLogRepository::recentLogs(int limit) const
{
    QList<DownloadLog> result;
    if (!m_database) {
        return result;
    }

    QSqlQuery query(m_database->database());
    query.prepare("SELECT log_id, peer_id, share_id, remote_path, local_path, "
                  "file_name, file_size, downloaded_at, success "
                  "FROM download_logs ORDER BY downloaded_at DESC LIMIT ?");
    query.addBindValue(qMax(1, limit));

    if (!query.exec()) {
        qWarning() << "Failed to query download logs:" << query.lastError().text();
        return result;
    }

    while (query.next()) {
        result.append(logFromQuery(query));
    }
    return result;
}

DownloadLog DownloadLogRepository::logFromQuery(const QSqlQuery& query)
{
    DownloadLog log;
    log.logId = query.value(0).toString();
    log.peerId = query.value(1).toString();
    log.shareId = query.value(2).toString();
    log.remotePath = query.value(3).toString();
    log.localPath = query.value(4).toString();
    log.fileName = query.value(5).toString();
    log.fileSize = query.value(6).toLongLong();
    log.downloadedAt = QDateTime::fromString(query.value(7).toString(), Qt::ISODate);
    log.success = query.value(8).toInt() != 0;
    return log;
}

} // namespace FengSui
