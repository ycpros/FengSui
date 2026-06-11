// TransferCenterPage.cpp
// 传输中心页面：展示历史和进行中的文件传输任务。

#include "ui/transfer_center/TransferCenterPage.h"

#include "core/CourierService.h"
#include "ui/UiStyle.h"

#include <QColor>
#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QUrl>
#include <QVBoxLayout>

namespace FengSui {

namespace {

constexpr int kAllFilterValue = -1;

enum Column {
    DirectionColumn = 0,
    FileNameColumn,
    PeerColumn,
    StatusColumn,
    ProgressColumn,
    SizeColumn,
    TimeColumn,
    ErrorColumn,
    ActionColumn,
    ColumnCount
};

QTableWidgetItem* readOnlyItem(const QString& text)
{
    auto* item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

int filterValue(TransferStatus status)
{
    return static_cast<int>(status);
}

} // namespace

TransferCenterPage::TransferCenterPage(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    renderTasks();
}

void TransferCenterPage::setCourierService(CourierService* service)
{
    if (m_courierService == service) {
        return;
    }
    if (m_courierService) {
        disconnect(m_courierService, nullptr, this, nullptr);
    }

    m_courierService = service;
    setupConnections();
    refreshTasks();
}

void TransferCenterPage::refreshTasks()
{
    m_tasks = m_courierService ? m_courierService->allTasks() : QList<TransferTask>();
    renderTasks();
}

void TransferCenterPage::onFilterChanged(int index)
{
    Q_UNUSED(index);
    renderTasks();
}

void TransferCenterPage::setupUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);

    auto* header = new QWidget(this);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(12);

    auto* titleBlock = new QWidget(header);
    auto* titleLayout = new QVBoxLayout(titleBlock);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(2);

    auto* titleLabel = new QLabel(QStringLiteral("传输中心"), titleBlock);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));
    titleLabel->setStyleSheet(UiStyle::pageTitleStyle());

    auto* subtitleLabel = new QLabel(QStringLiteral("统一查看文件收发、进度和失败原因"), titleBlock);
    subtitleLabel->setStyleSheet(UiStyle::mutedTextStyle());
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(subtitleLabel);

    m_filterCombo = new QComboBox(header);
    m_filterCombo->setObjectName(QStringLiteral("transferFilterCombo"));
    m_filterCombo->setFixedWidth(140);
    m_filterCombo->addItem(QStringLiteral("全部"), kAllFilterValue);
    m_filterCombo->addItem(QStringLiteral("等待中"), filterValue(TransferStatus::Waiting));
    m_filterCombo->addItem(QStringLiteral("传输中"), filterValue(TransferStatus::Transferring));
    m_filterCombo->addItem(QStringLiteral("已完成"), filterValue(TransferStatus::Completed));
    m_filterCombo->addItem(QStringLiteral("失败"), filterValue(TransferStatus::Failed));
    m_filterCombo->addItem(QStringLiteral("已拒绝"), filterValue(TransferStatus::Rejected));
    m_filterCombo->addItem(QStringLiteral("已取消"), filterValue(TransferStatus::Cancelled));

    m_searchEdit = new QLineEdit(header);
    m_searchEdit->setObjectName(QStringLiteral("transferSearchEdit"));
    m_searchEdit->setPlaceholderText(QStringLiteral("搜索文件或设备"));
    m_searchEdit->setFixedWidth(220);

    headerLayout->addWidget(titleBlock, 1);
    headerLayout->addWidget(m_searchEdit);
    headerLayout->addWidget(m_filterCombo);

    m_table = new QTableWidget(this);
    m_table->setObjectName(QStringLiteral("transferTable"));
    m_table->setColumnCount(ColumnCount);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("方向"),
        QStringLiteral("文件"),
        QStringLiteral("对端"),
        QStringLiteral("状态"),
        QStringLiteral("进度"),
        QStringLiteral("大小"),
        QStringLiteral("时间"),
        QStringLiteral("原因"),
        QStringLiteral("操作"),
    });
    m_table->setAlternatingRowColors(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(DirectionColumn, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(FileNameColumn, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(PeerColumn, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(StatusColumn, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ProgressColumn, QHeaderView::Fixed);
    m_table->setColumnWidth(ProgressColumn, 150);
    m_table->horizontalHeader()->setSectionResizeMode(SizeColumn, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(TimeColumn, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ErrorColumn, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(ActionColumn, QHeaderView::ResizeToContents);

    m_emptyLabel = new QLabel(QStringLiteral("暂无传输任务"), this);
    m_emptyLabel->setObjectName(QStringLiteral("transferEmptyLabel"));
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet("color: #999; font-size: 14px;");

    layout->addWidget(header);
    layout->addWidget(m_table, 1);
    layout->addWidget(m_emptyLabel, 1);

    connect(m_filterCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &TransferCenterPage::onFilterChanged);
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this]() {
        renderTasks();
    });
}

void TransferCenterPage::setupConnections()
{
    if (!m_courierService) {
        return;
    }

    connect(m_courierService,
            &CourierService::transferProgress,
            this,
            [this](const QString&, qint64, qint64) { refreshTasks(); });
    connect(m_courierService,
            &CourierService::transferAccepted,
            this,
            [this](const QString&) { refreshTasks(); });
    connect(m_courierService,
            &CourierService::transferRejected,
            this,
            [this](const QString&, const QString&) { refreshTasks(); });
    connect(m_courierService,
            &CourierService::transferCompleted,
            this,
            [this](const QString&) { refreshTasks(); });
    connect(m_courierService,
            &CourierService::transferFailed,
            this,
            [this](const QString&, const QString&) { refreshTasks(); });
}

void TransferCenterPage::renderTasks()
{
    QList<TransferTask> visibleTasks;
    for (const TransferTask& task : m_tasks) {
        if (taskMatchesFilter(task)) {
            visibleTasks.append(task);
        }
    }

    m_table->setRowCount(0);
    m_table->setVisible(!visibleTasks.isEmpty());
    m_emptyLabel->setVisible(visibleTasks.isEmpty());

    for (int row = 0; row < visibleTasks.size(); ++row) {
        const TransferTask& task = visibleTasks.at(row);
        m_table->insertRow(row);
        m_table->setRowHeight(row, 44);

        m_table->setItem(row, DirectionColumn, readOnlyItem(directionText(task.direction)));
        m_table->setItem(row, FileNameColumn, readOnlyItem(task.fileName));
        m_table->setItem(row, PeerColumn, readOnlyItem(task.peerId));
        auto* statusItem = readOnlyItem(statusText(task.status));
        switch (task.status) {
        case TransferStatus::Completed:
            statusItem->setForeground(QColor(QStringLiteral("#187a3b")));
            break;
        case TransferStatus::Failed:
        case TransferStatus::Rejected:
        case TransferStatus::Cancelled:
            statusItem->setForeground(QColor(QStringLiteral("#b42318")));
            break;
        case TransferStatus::Transferring:
            statusItem->setForeground(QColor(QStringLiteral("#0f6f70")));
            break;
        case TransferStatus::Waiting:
            statusItem->setForeground(QColor(QStringLiteral("#8a5a00")));
            break;
        }
        m_table->setItem(row, StatusColumn, statusItem);
        m_table->setCellWidget(row, ProgressColumn, createProgressCell(task));
        m_table->setItem(row, SizeColumn, readOnlyItem(sizeText(task.fileSize)));
        m_table->setItem(row, TimeColumn, readOnlyItem(timeText(task.createdAt)));
        auto* errorItem = readOnlyItem(task.errorMessage);
        if (!task.errorMessage.trimmed().isEmpty()) {
            errorItem->setToolTip(task.errorMessage);
        }
        m_table->setItem(row, ErrorColumn, errorItem);
        m_table->setCellWidget(row, ActionColumn, createActionCell(task));
    }
}

bool TransferCenterPage::taskMatchesFilter(const TransferTask& task) const
{
    if (!m_filterCombo) {
        return true;
    }
    const int value = m_filterCombo->currentData().toInt();
    if (value == kAllFilterValue) {
        // continue into search filter below
    } else if (static_cast<int>(task.status) != value) {
        return false;
    }

    const QString query = m_searchEdit ? m_searchEdit->text().trimmed() : QString();
    if (query.isEmpty()) {
        return true;
    }
    return task.fileName.contains(query, Qt::CaseInsensitive)
        || task.peerId.contains(query, Qt::CaseInsensitive)
        || task.errorMessage.contains(query, Qt::CaseInsensitive);
}

QWidget* TransferCenterPage::createProgressCell(const TransferTask& task)
{
    auto* container = new QWidget(m_table);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(2);

    auto* progress = new QProgressBar(container);
    progress->setObjectName(QStringLiteral("transferProgressBar_%1").arg(task.transferId));
    progress->setRange(0, 100);
    const int value = task.fileSize > 0
        ? static_cast<int>((task.transferredBytes * 100) / task.fileSize)
        : (task.status == TransferStatus::Completed ? 100 : 0);
    progress->setValue(qBound(0, value, 100));
    progress->setTextVisible(true);

    auto* detail = new QLabel(
        QStringLiteral("%1 / %2").arg(sizeText(task.transferredBytes),
                                      sizeText(task.fileSize)),
        container);
    detail->setAlignment(Qt::AlignCenter);
    detail->setStyleSheet("font-size: 11px; color: #666;");

    layout->addWidget(progress);
    layout->addWidget(detail);
    return container;
}

QWidget* TransferCenterPage::createActionCell(const TransferTask& task)
{
    auto* container = new QWidget(m_table);
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(0);

    auto* button = new QPushButton(QStringLiteral("打开目录"), container);
    button->setObjectName(QStringLiteral("openTransferDir_%1").arg(task.transferId));
    button->setStyleSheet(UiStyle::secondaryButtonStyle());
    const QFileInfo fileInfo(task.filePath);
    const bool canOpen = task.status == TransferStatus::Completed
        && !task.filePath.trimmed().isEmpty()
        && fileInfo.exists();
    button->setEnabled(canOpen);
    connect(button, &QPushButton::clicked, this, [this, task]() {
        openTaskDirectory(task);
    });

    layout->addWidget(button);
    return container;
}

void TransferCenterPage::openTaskDirectory(const TransferTask& task)
{
    const QFileInfo fileInfo(task.filePath);
    if (!fileInfo.exists()) {
        return;
    }

    const QString directoryPath = fileInfo.absoluteDir().absolutePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(directoryPath));
}

QString TransferCenterPage::directionText(TransferDirection direction)
{
    return direction == TransferDirection::Upload
        ? QStringLiteral("发出")
        : QStringLiteral("接收");
}

QString TransferCenterPage::statusText(TransferStatus status)
{
    switch (status) {
    case TransferStatus::Waiting:
        return QStringLiteral("等待中");
    case TransferStatus::Rejected:
        return QStringLiteral("已拒绝");
    case TransferStatus::Transferring:
        return QStringLiteral("传输中");
    case TransferStatus::Completed:
        return QStringLiteral("已完成");
    case TransferStatus::Failed:
        return QStringLiteral("失败");
    case TransferStatus::Cancelled:
        return QStringLiteral("已取消");
    }
    return QStringLiteral("等待中");
}

QString TransferCenterPage::sizeText(qint64 bytes)
{
    if (bytes < 1024) {
        return QStringLiteral("%1 B").arg(bytes);
    }

    double value = static_cast<double>(bytes) / 1024.0;
    QString unit = QStringLiteral("KB");
    if (value >= 1024.0) {
        value /= 1024.0;
        unit = QStringLiteral("MB");
    }
    if (value >= 1024.0) {
        value /= 1024.0;
        unit = QStringLiteral("GB");
    }
    return QStringLiteral("%1 %2").arg(value, 0, 'f', 1).arg(unit);
}

QString TransferCenterPage::timeText(const QDateTime& time)
{
    if (!time.isValid()) {
        return QStringLiteral("-");
    }
    return time.toLocalTime().toString(QStringLiteral("MM-dd HH:mm"));
}

} // namespace FengSui
