// TransferRepository.cpp
// 传输任务存储仓库实现。

#include "storage/TransferRepository.h"

#include <QDateTime>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "storage/Database.h"

namespace FengSui {

TransferRepository::TransferRepository(Database* database, QObject* parent)
    : QObject(parent)
    , m_database(database)
{
}

bool TransferRepository::saveTask(const TransferTask& task)
{
    if (!m_database) {
        qWarning() << "TransferRepository has no database";
        return false;
    }

    QSqlQuery query(m_database->database());
    // INSERT OR REPLACE 支持幂等写入：同一 transferId 的后续状态更新可覆盖
    query.prepare("INSERT OR REPLACE INTO transfer_tasks "
                  "(transfer_id, direction, peer_id, file_name, file_path, "
                  " file_size, transferred_bytes, sha256, status, error_message, "
                  " created_at, completed_at) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(task.transferId);
    query.addBindValue(directionToString(task.direction));
    query.addBindValue(task.peerId);
    query.addBindValue(task.fileName);
    query.addBindValue(task.filePath);
    query.addBindValue(task.fileSize);
    query.addBindValue(task.transferredBytes);
    query.addBindValue(task.sha256);
    query.addBindValue(statusToString(task.status));
    query.addBindValue(task.errorMessage);
    query.addBindValue(task.createdAt.toString(Qt::ISODate));
    query.addBindValue(task.completedAt.isValid()
                       ? task.completedAt.toString(Qt::ISODate)
                       : QVariant());

    if (!query.exec()) {
        qWarning() << "Failed to save transfer task:" << query.lastError().text();
        return false;
    }

    return true;
}

QList<TransferTask> TransferRepository::getAllTasks() const
{
    QList<TransferTask> result;

    if (!m_database) {
        return result;
    }

    QSqlQuery query(m_database->database());
    query.prepare("SELECT transfer_id, direction, peer_id, file_name, file_path, "
                  "file_size, transferred_bytes, sha256, status, error_message, "
                  "created_at, completed_at "
                  "FROM transfer_tasks "
                  "ORDER BY created_at DESC");

    if (!query.exec()) {
        qWarning() << "Failed to get all transfer tasks:" << query.lastError().text();
        return result;
    }

    while (query.next()) {
        result.append(taskFromQuery(query));
    }

    return result;
}

QList<TransferTask> TransferRepository::getTasksByStatus(TransferStatus status) const
{
    QList<TransferTask> result;

    if (!m_database) {
        return result;
    }

    QSqlQuery query(m_database->database());
    query.prepare("SELECT transfer_id, direction, peer_id, file_name, file_path, "
                  "file_size, transferred_bytes, sha256, status, error_message, "
                  "created_at, completed_at "
                  "FROM transfer_tasks "
                  "WHERE status = ? "
                  "ORDER BY created_at DESC");
    query.addBindValue(statusToString(status));

    if (!query.exec()) {
        qWarning() << "Failed to get transfer tasks by status:" << query.lastError().text();
        return result;
    }

    while (query.next()) {
        result.append(taskFromQuery(query));
    }

    return result;
}

QList<TransferTask> TransferRepository::getActiveTasks() const
{
    QList<TransferTask> result;

    if (!m_database) {
        return result;
    }

    QSqlQuery query(m_database->database());
    query.prepare("SELECT transfer_id, direction, peer_id, file_name, file_path, "
                  "file_size, transferred_bytes, sha256, status, error_message, "
                  "created_at, completed_at "
                  "FROM transfer_tasks "
                  "WHERE status IN ('waiting', 'transferring') "
                  "ORDER BY created_at DESC");

    if (!query.exec()) {
        qWarning() << "Failed to get active transfer tasks:" << query.lastError().text();
        return result;
    }

    while (query.next()) {
        result.append(taskFromQuery(query));
    }

    return result;
}

std::optional<TransferTask> TransferRepository::getTask(const QString& transferId) const
{
    if (!m_database) {
        return std::nullopt;
    }

    QSqlQuery query(m_database->database());
    query.prepare("SELECT transfer_id, direction, peer_id, file_name, file_path, "
                  "file_size, transferred_bytes, sha256, status, error_message, "
                  "created_at, completed_at "
                  "FROM transfer_tasks WHERE transfer_id = ?");
    query.addBindValue(transferId);

    if (!query.exec()) {
        qWarning() << "Failed to get transfer task:" << query.lastError().text();
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    return taskFromQuery(query);
}

bool TransferRepository::updateProgress(const QString& transferId,
                                        qint64 transferredBytes)
{
    if (!m_database) {
        return false;
    }

    QSqlQuery query(m_database->database());
    query.prepare("UPDATE transfer_tasks SET transferred_bytes = ?, "
                  "status = 'transferring' WHERE transfer_id = ?");
    query.addBindValue(transferredBytes);
    query.addBindValue(transferId);

    if (!query.exec()) {
        qWarning() << "Failed to update transfer progress:" << query.lastError().text();
        return false;
    }

    return true;
}

bool TransferRepository::updateStatus(const QString& transferId,
                                      TransferStatus status,
                                      const QString& errorMessage)
{
    if (!m_database) {
        return false;
    }

    QSqlQuery query(m_database->database());

    // 完成态自动写入完成时间
    if (status == TransferStatus::Completed
        || status == TransferStatus::Failed
        || status == TransferStatus::Cancelled
        || status == TransferStatus::Rejected) {

        query.prepare("UPDATE transfer_tasks SET status = ?, error_message = ?, "
                      "completed_at = ? WHERE transfer_id = ?");
        query.addBindValue(statusToString(status));
        query.addBindValue(errorMessage);
        query.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
        query.addBindValue(transferId);
    } else {
        query.prepare("UPDATE transfer_tasks SET status = ?, error_message = ? "
                      "WHERE transfer_id = ?");
        query.addBindValue(statusToString(status));
        query.addBindValue(errorMessage);
        query.addBindValue(transferId);
    }

    if (!query.exec()) {
        qWarning() << "Failed to update transfer status:" << query.lastError().text();
        return false;
    }

    return true;
}

bool TransferRepository::markCompleted(const QString& transferId,
                                       const QString& sha256)
{
    if (!m_database) {
        return false;
    }

    QSqlQuery query(m_database->database());
    query.prepare("UPDATE transfer_tasks SET status = 'completed', "
                  "sha256 = ?, completed_at = ? WHERE transfer_id = ?");
    query.addBindValue(sha256);
    query.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    query.addBindValue(transferId);

    if (!query.exec()) {
        qWarning() << "Failed to mark transfer completed:" << query.lastError().text();
        return false;
    }

    return true;
}

// ---- Private Helpers ----

TransferStatus TransferRepository::statusFromString(const QString& s)
{
    if (s == QStringLiteral("waiting"))      return TransferStatus::Waiting;
    if (s == QStringLiteral("rejected"))     return TransferStatus::Rejected;
    if (s == QStringLiteral("transferring")) return TransferStatus::Transferring;
    if (s == QStringLiteral("completed"))    return TransferStatus::Completed;
    if (s == QStringLiteral("failed"))       return TransferStatus::Failed;
    if (s == QStringLiteral("cancelled"))    return TransferStatus::Cancelled;
    return TransferStatus::Waiting;
}

QString TransferRepository::statusToString(TransferStatus s)
{
    switch (s) {
    case TransferStatus::Waiting:      return QStringLiteral("waiting");
    case TransferStatus::Rejected:     return QStringLiteral("rejected");
    case TransferStatus::Transferring: return QStringLiteral("transferring");
    case TransferStatus::Completed:    return QStringLiteral("completed");
    case TransferStatus::Failed:       return QStringLiteral("failed");
    case TransferStatus::Cancelled:    return QStringLiteral("cancelled");
    }
    return QStringLiteral("waiting");
}

TransferDirection TransferRepository::directionFromString(const QString& s)
{
    if (s == QStringLiteral("upload"))   return TransferDirection::Upload;
    if (s == QStringLiteral("download")) return TransferDirection::Download;
    return TransferDirection::Upload;
}

QString TransferRepository::directionToString(TransferDirection d)
{
    switch (d) {
    case TransferDirection::Upload:   return QStringLiteral("upload");
    case TransferDirection::Download: return QStringLiteral("download");
    }
    return QStringLiteral("upload");
}

TransferTask TransferRepository::taskFromQuery(const QSqlQuery& query)
{
    TransferTask task;
    task.transferId = query.value(0).toString();
    task.direction = directionFromString(query.value(1).toString());
    task.peerId = query.value(2).toString();
    task.fileName = query.value(3).toString();
    task.filePath = query.value(4).toString();
    task.fileSize = query.value(5).toLongLong();
    task.transferredBytes = query.value(6).toLongLong();
    task.sha256 = query.value(7).toString();
    task.status = statusFromString(query.value(8).toString());
    task.errorMessage = query.value(9).toString();
    task.createdAt = QDateTime::fromString(query.value(10).toString(), Qt::ISODate);
    task.completedAt = QDateTime::fromString(query.value(11).toString(), Qt::ISODate);
    return task;
}

} // namespace FengSui
