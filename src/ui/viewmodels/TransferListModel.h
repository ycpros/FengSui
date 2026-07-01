// TransferListModel.h
// 传输任务列表模型：向 QML 暴露传输任务行。
// 关键：进度/状态用 transferId→行 哈希 O(1) 定位，只对变化 role 发 dataChanged，
// 严禁每次进度信号都 reset 整表（高频刷新）。

#pragma once

#include "models/TransferTask.h"

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QString>

namespace FengSui {

class TransferListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCountProperty NOTIFY countChanged)

public:
    enum Roles {
        TransferIdRole = Qt::UserRole + 1,
        DirectionRole,       // int(TransferDirection)
        FileNameRole,
        PeerNameRole,        // 对端显示名（VM 解析后写入）
        StatusRole,          // int(TransferStatus)
        TransferredRole,     // 已传字节
        SizeRole,            // 总字节
        ProgressRole,        // 0.0~1.0
        ErrorRole,
        CreatedAtRole,
        CompletedAtRole
    };

    explicit TransferListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int rowCountProperty() const { return m_rows.size(); }

    // 一行的展示数据（任务 + 已解析的对端显示名）
    struct Row {
        TransferTask task;
        QString      peerName;
    };

    // 全量重置
    void resetRows(const QList<Row>& rows);
    // 增量 upsert 一行（新任务或整体刷新某任务）
    void upsert(const Row& row);
    // 仅更新进度（只发 Progress/Transferred/Size role）
    void updateProgress(const QString& transferId, qint64 done, qint64 total);
    // 仅更新状态与错误
    void updateStatus(const QString& transferId, TransferStatus status,
                      const QString& error = QString());

signals:
    void countChanged();

private:
    QList<Row> m_rows;
    QHash<QString, int> m_indexById;   // transferId → 行号

    static double progressOf(const TransferTask& t);
    QModelIndex indexForId(const QString& transferId) const;
};

} // namespace FengSui
