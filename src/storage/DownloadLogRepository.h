// DownloadLogRepository.h
// SQLite download_logs 表访问封装。

#pragma once

#include "models/DownloadLog.h"

#include <QList>
#include <QObject>
#include <QString>

class QSqlQuery;

namespace FengSui {

class Database;

// 共享文件下载日志仓库。
// 只负责 download_logs 表读写，下载成功/失败语义由 ShareService 决定。
class DownloadLogRepository : public QObject {
    Q_OBJECT

public:
    explicit DownloadLogRepository(Database* database, QObject* parent = nullptr);

    // 保存一条下载日志。
    bool saveLog(const DownloadLog& log);

    // 返回最近下载日志，按 downloaded_at 倒序。
    QList<DownloadLog> recentLogs(int limit = 50) const;

private:
    static DownloadLog logFromQuery(const QSqlQuery& query);

    Database* m_database = nullptr;  // 数据库连接提供者，生命周期由 Application 管理。
};

} // namespace FengSui
