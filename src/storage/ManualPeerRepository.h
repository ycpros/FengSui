// ManualPeerRepository.h
// 手动添加设备持久化仓库：保存用户成功探测过的 IP:端口。

#pragma once

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>
#include <optional>

class QSqlQuery;

namespace FengSui {

class Database;

struct ManualPeer {
    QString name;
    QString host;
    quint16 port = 0;
    QDateTime lastSuccessAt;
};

class ManualPeerRepository : public QObject {
    Q_OBJECT

public:
    explicit ManualPeerRepository(Database* database, QObject* parent = nullptr);

    bool upsertManualPeer(const ManualPeer& peer);
    QList<ManualPeer> getAllManualPeers() const;
    std::optional<ManualPeer> getManualPeer(const QString& host,
                                            quint16 port) const;

private:
    static ManualPeer manualPeerFromQuery(const QSqlQuery& query);

    Database* m_database = nullptr;
};

} // namespace FengSui
