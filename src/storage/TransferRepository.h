// TransferRepository.h
// 传输任务存储仓库：管理 transfer_tasks 表 CRUD，支持状态查询和进度更新。

#pragma once

#include "models/TransferTask.h"

#include <QObject>
#include <QString>
#include <QList>
#include <optional>

class QSqlQuery;

namespace FengSui {

class Database;

// 传输任务持久化仓库。
// 负责 transfer_tasks 表的增删查改，不感知网络层或 UI 层。
class TransferRepository : public QObject {
    Q_OBJECT

public:
    // 创建传输任务仓库。
    // database: 数据库管理器实例，生命周期必须长于本对象。
    // parent: Qt 父对象。
    explicit TransferRepository(Database* database, QObject* parent = nullptr);

    // 保存传输任务（INSERT OR REPLACE，便于状态更新的幂等写入）。
    // 返回 true 表示成功写入。
    bool saveTask(const TransferTask& task);

    // 获取所有传输任务，按 created_at 降序排列（最新任务在前）。
    QList<TransferTask> getAllTasks() const;

    // 获取指定状态的传输任务列表。
    QList<TransferTask> getTasksByStatus(TransferStatus status) const;

    // 获取进行中的传输任务（状态为 waiting 或 transferring）。
    QList<TransferTask> getActiveTasks() const;

    // 按 transferId 查找单条传输任务。
    std::optional<TransferTask> getTask(const QString& transferId) const;

    // 更新已传输字节数（进度）。
    bool updateProgress(const QString& transferId, qint64 transferredBytes);

    // 更新传输任务的完整状态，同时可设置错误信息和完成时间。
    // errorMessage: 仅失败态时需要填写，传入空字符串则保留原值。
    bool updateStatus(const QString& transferId,
                      TransferStatus status,
                      const QString& errorMessage = QString());

    // 标记传输完成（同时写入 SHA-256 和完成时间）。
    bool markCompleted(const QString& transferId,
                       const QString& sha256 = QString());

private:
    // 枚举 ↔ DB 字符串互转辅助方法
    static TransferStatus statusFromString(const QString& s);
    static QString statusToString(TransferStatus s);
    static TransferDirection directionFromString(const QString& s);
    static QString directionToString(TransferDirection d);

    // 从 QSqlQuery 当前行构造 TransferTask 结构体。
    static TransferTask taskFromQuery(const QSqlQuery& query);

    Database* m_database = nullptr;
};

} // namespace FengSui
