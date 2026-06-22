// AccessGrantRepository.h
// SQLite access_grants 表访问封装。

#pragma once

#include "models/AccessGrant.h"

#include <QObject>
#include <QString>
#include <optional>

class QSqlQuery;

namespace FengSui {

class Database;

// 共享访问授权仓库。
// 只负责 access_grants 表读写，不做弹窗、路径或网络协议判断。
class AccessGrantRepository : public QObject {
    Q_OBJECT

public:
    explicit AccessGrantRepository(Database* database, QObject* parent = nullptr);

    // 保存授权记录；同 peer/share 重复授权时覆盖 granted_at 和 remember。
    bool saveGrant(const AccessGrant& grant);

    // 查询指定 peer 对指定 share 的持久授权。
    std::optional<AccessGrant> grantFor(const QString& peerId,
                                        const QString& shareId) const;

    // 判断指定 peer 对指定 share 是否已有持久授权。
    bool hasGrant(const QString& peerId, const QString& shareId) const;

private:
    static AccessGrant grantFromQuery(const QSqlQuery& query);

    Database* m_database = nullptr;  // 数据库连接提供者，生命周期由 Application 管理。
};

} // namespace FengSui
