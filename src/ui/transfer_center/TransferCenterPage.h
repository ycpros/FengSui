// TransferCenterPage.h
// 传输中心：展示所有文件传输任务的统一面板。

#pragma once

#include "models/TransferTask.h"

#include <QList>
#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QTableWidget;

namespace FengSui {

class CourierService;

class TransferCenterPage : public QWidget {
    Q_OBJECT

public:
    explicit TransferCenterPage(QWidget* parent = nullptr);

    // 注入文件传输服务。页面只通过 core 层查询任务快照，不直接访问 SQLite。
    void setCourierService(CourierService* service);

private slots:
    void refreshTasks();
    void onFilterChanged(int index);

private:
    void setupUi();
    void setupConnections();
    void renderTasks();
    bool taskMatchesFilter(const TransferTask& task) const;
    QWidget* createProgressCell(const TransferTask& task);
    QWidget* createActionCell(const TransferTask& task);
    void openTaskDirectory(const TransferTask& task);

    static QString directionText(TransferDirection direction);
    static QString statusText(TransferStatus status);
    static QString sizeText(qint64 bytes);
    static QString timeText(const QDateTime& time);

    CourierService* m_courierService = nullptr;
    QList<TransferTask> m_tasks;
    QComboBox* m_filterCombo = nullptr;
    QLineEdit* m_searchEdit = nullptr;
    QTableWidget* m_table = nullptr;
    QLabel* m_emptyLabel = nullptr;
};

} // namespace FengSui
