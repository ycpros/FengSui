// PeerListModel.h
// 在线设备列表模型：向 QML 暴露 PeerInfo 行，支持按 peerId 增量 upsert/remove。
// 维护 peerId→行号 哈希做 O(1) 定位，只对变化行发 dataChanged，避免整表 reset。

#pragma once

#include "models/PeerInfo.h"

#include <QAbstractListModel>
#include <QHash>
#include <QList>

namespace FengSui {

class PeerListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCountProperty NOTIFY countChanged)

public:
    enum Roles {
        PeerIdRole = Qt::UserRole + 1,
        DisplayNameRole,
        DeviceNameRole,
        IpRole,
        PortRole,
        OsRole,
        OnlineRole,
        ShareEnabledRole,
        EndpointRole      // "ip:port" 组合，便于 QML 直接显示
    };

    explicit PeerListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int rowCountProperty() const { return m_rows.size(); }

    // 全量重置（初始化快照）
    void resetFrom(const QList<PeerInfo>& peers);
    // 增量新增或更新一台设备
    void upsert(const PeerInfo& peer);
    // 移除一台设备
    void remove(const QString& peerId);
    // 按行取 PeerInfo（供 QML 双击打开会话时回传）
    Q_INVOKABLE FengSui::PeerInfo at(int row) const;

signals:
    void countChanged();

private:
    QList<PeerInfo> m_rows;
    QHash<QString, int> m_indexById;   // peerId → 行号

    void rebuildIndex();
};

} // namespace FengSui
