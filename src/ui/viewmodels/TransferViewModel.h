// TransferViewModel.h
// 传输中心 ViewModel：连接 CourierService 全部传输信号，维护任务列表模型，
// 提供搜索关键词与状态筛选（经内置 QSortFilterProxyModel 过滤），以及接受/拒绝/取消命令。

#pragma once

#include "ui/viewmodels/TransferListModel.h"

#include <QObject>
#include <QSortFilterProxyModel>
#include <QString>

namespace FengSui {

class CourierService;
class BeaconService;

// 过滤代理：按文件名/对端名关键词 + 状态筛选。
class TransferFilterModel : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(QString keyword READ keyword WRITE setKeyword NOTIFY keywordChanged)
    Q_PROPERTY(int statusFilter READ statusFilter WRITE setStatusFilter NOTIFY statusFilterChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit TransferFilterModel(QObject* parent = nullptr);

    QString keyword() const { return m_keyword; }
    void setKeyword(const QString& kw);

    // -1 表示全部；否则为 int(TransferStatus)
    int statusFilter() const { return m_statusFilter; }
    void setStatusFilter(int s);

    int count() const { return rowCount(); }

signals:
    void keywordChanged();
    void statusFilterChanged();
    void countChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    QString m_keyword;
    int     m_statusFilter = -1;
};

class TransferViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(TransferFilterModel* tasks READ tasks CONSTANT)

public:
    explicit TransferViewModel(QObject* parent = nullptr);

    void bind(CourierService* courier, BeaconService* beacon);

    TransferFilterModel* tasks() const { return m_proxy; }

    Q_INVOKABLE void acceptTransfer(const QString& transferId);
    Q_INVOKABLE void rejectTransfer(const QString& transferId);
    Q_INVOKABLE void cancelTransfer(const QString& transferId);
    // 刷新全量快照（进入页面时可调用）
    Q_INVOKABLE void reload();

signals:
    // 收到对方发来的传输请求（QML 弹出接受/拒绝对话框）
    void incomingRequest(const QString& transferId,
                         const QString& peerName,
                         const QString& fileName,
                         qint64 fileSize);

private slots:
    void onTransferRequested(const TransferTask& task);
    void onTransferProgress(const QString& id, qint64 done, qint64 total);
    void onTransferAccepted(const QString& id);
    void onTransferRejected(const QString& id, const QString& reason);
    void onTransferCompleted(const QString& id);
    void onTransferFailed(const QString& id, const QString& error);

private:
    // 解析对端显示名（在线设备优先，否则回退 peerId）
    QString resolvePeerName(const QString& peerId) const;
    TransferListModel::Row toRow(const TransferTask& task) const;

    TransferListModel*   m_model = nullptr;
    TransferFilterModel* m_proxy = nullptr;
    CourierService*      m_courier = nullptr;
    BeaconService*       m_beacon = nullptr;
};

} // namespace FengSui
