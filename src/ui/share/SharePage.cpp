// SharePage.cpp
// Shared files shell: online browsing and local share management placeholders.

#include "ui/share/SharePage.h"

#include "core/ShareService.h"
#include "ui/UiStyle.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSize>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace FengSui {

namespace {

QTableWidgetItem* readOnlyItem(const QString& text)
{
    auto* item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

} // namespace

SharePage::SharePage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("sharePage"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);

    auto* titleLabel = new QLabel(QStringLiteral("共享文件"), this);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));
    titleLabel->setStyleSheet(UiStyle::pageTitleStyle());

    auto* subtitleLabel = new QLabel(
        QStringLiteral("浏览同网段设备的共享目录，或管理本机将来对外提供的共享。"),
        this);
    subtitleLabel->setObjectName(QStringLiteral("pageSubtitle"));
    subtitleLabel->setStyleSheet(UiStyle::mutedTextStyle());

    auto* tabWidget = new QTabWidget(this);
    tabWidget->setObjectName(QStringLiteral("shareTabWidget"));

    auto* browseTab = new QWidget(tabWidget);
    auto* browseLayout = new QVBoxLayout(browseTab);
    browseLayout->setContentsMargins(16, 16, 16, 16);
    browseLayout->setSpacing(12);

    auto* browseHint = new QLabel(QStringLiteral("共享服务尚未接入，当前显示浏览器壳层。"),
                                  browseTab);
    browseHint->setObjectName(QStringLiteral("shareBrowseHintLabel"));
    browseHint->setStyleSheet(UiStyle::pillStyle(QStringLiteral("#eef4ff"),
                                                 QStringLiteral("#175cd3")));

    auto* splitter = new QSplitter(Qt::Horizontal, browseTab);
    splitter->setHandleWidth(1);

    auto* sourcePanel = new QWidget(splitter);
    auto* sourceLayout = new QVBoxLayout(sourcePanel);
    sourceLayout->setContentsMargins(0, 0, 12, 0);
    sourceLayout->setSpacing(8);
    auto* sourceTitle = new QLabel(QStringLiteral("在线共享源"), sourcePanel);
    sourceTitle->setStyleSheet(UiStyle::sectionTitleStyle());
    auto* sourceList = new QListWidget(sourcePanel);
    sourceList->setObjectName(QStringLiteral("shareSourceList"));
    sourceList->addItem(QStringLiteral("暂无在线共享源"));
    sourceList->item(0)->setFlags(Qt::ItemIsEnabled);
    sourceLayout->addWidget(sourceTitle);
    sourceLayout->addWidget(sourceList, 1);

    auto* browserPanel = new QWidget(splitter);
    auto* browserLayout = new QVBoxLayout(browserPanel);
    browserLayout->setContentsMargins(12, 0, 0, 0);
    browserLayout->setSpacing(8);
    auto* breadcrumb = new QLabel(QStringLiteral("位置：/"), browserPanel);
    breadcrumb->setObjectName(QStringLiteral("shareBreadcrumbLabel"));
    breadcrumb->setStyleSheet(UiStyle::mutedTextStyle());
    auto* fileTable = new QTableWidget(browserPanel);
    fileTable->setObjectName(QStringLiteral("shareFileTable"));
    fileTable->setColumnCount(4);
    fileTable->setHorizontalHeaderLabels({
        QStringLiteral("名称"),
        QStringLiteral("类型"),
        QStringLiteral("大小"),
        QStringLiteral("修改时间"),
    });
    fileTable->verticalHeader()->setVisible(false);
    fileTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    fileTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    fileTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    fileTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    fileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    fileTable->setRowCount(1);
    fileTable->setItem(0, 0, readOnlyItem(QStringLiteral("选择共享源后显示目录内容")));
    fileTable->setItem(0, 1, readOnlyItem(QStringLiteral("-")));
    fileTable->setItem(0, 2, readOnlyItem(QStringLiteral("-")));
    fileTable->setItem(0, 3, readOnlyItem(QStringLiteral("-")));
    browserLayout->addWidget(breadcrumb);
    browserLayout->addWidget(fileTable, 1);

    splitter->addWidget(sourcePanel);
    splitter->addWidget(browserPanel);
    splitter->setSizes({260, 620});

    browseLayout->addWidget(browseHint);
    browseLayout->addWidget(splitter, 1);

    auto* myTab = new QWidget(tabWidget);
    auto* myLayout = new QVBoxLayout(myTab);
    myLayout->setContentsMargins(16, 16, 16, 16);
    myLayout->setSpacing(12);

    auto* toolbar = new QHBoxLayout();
    auto* myTitle = new QLabel(QStringLiteral("我的共享目录"), myTab);
    myTitle->setStyleSheet(UiStyle::sectionTitleStyle());
    auto* addButton = new QPushButton(QStringLiteral("添加共享"), myTab);
    addButton->setObjectName(QStringLiteral("shareAddButton"));
    addButton->setProperty("primary", true);
    addButton->setStyleSheet(UiStyle::primaryButtonStyle());
    toolbar->addWidget(myTitle);
    toolbar->addStretch();
    toolbar->addWidget(addButton);

    m_serviceHint = new QLabel(QStringLiteral("共享服务尚未接入，当前仅展示界面壳层。"), myTab);
    m_serviceHint->setObjectName(QStringLiteral("shareServiceHintLabel"));
    m_serviceHint->setStyleSheet(UiStyle::pillStyle(QStringLiteral("#fff4d6"),
                                                    QStringLiteral("#8a5a00")));
    m_serviceHint->setVisible(false);

    m_myShareList = new QListWidget(myTab);
    m_myShareList->setObjectName(QStringLiteral("myShareList"));
    m_myShareList->setFocusPolicy(Qt::NoFocus);
    m_myShareList->setSelectionMode(QAbstractItemView::NoSelection);
    renderMyShares();

    auto* controlsPreview = new QWidget(myTab);
    auto* controlsLayout = new QHBoxLayout(controlsPreview);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    auto* togglePreview = new QCheckBox(QStringLiteral("启用共享"), controlsPreview);
    togglePreview->setObjectName(QStringLiteral("shareEnablePreviewCheck"));
    togglePreview->setEnabled(false);
    auto* removePreview = new QPushButton(QStringLiteral("删除"), controlsPreview);
    removePreview->setObjectName(QStringLiteral("shareRemovePreviewButton"));
    removePreview->setEnabled(false);
    controlsLayout->addWidget(togglePreview);
    controlsLayout->addWidget(removePreview);
    controlsLayout->addStretch();

    tabWidget->addTab(browseTab, QStringLiteral("浏览共享"));
    tabWidget->addTab(myTab, QStringLiteral("我的共享"));

    myLayout->addLayout(toolbar);
    myLayout->addWidget(m_serviceHint);
    myLayout->addWidget(m_myShareList, 1);
    myLayout->addWidget(controlsPreview);

    layout->addWidget(titleLabel);
    layout->addWidget(subtitleLabel);
    layout->addWidget(tabWidget, 1);

    connect(addButton, &QPushButton::clicked, this, [this]() {
        if (!m_shareService) {
            showServiceMessage(QStringLiteral("共享服务尚未接入，当前仅展示界面壳层。"));
            return;
        }

        const QString directory = QFileDialog::getExistingDirectory(
            this,
            QStringLiteral("选择共享目录"));
        if (directory.trimmed().isEmpty()) {
            return;
        }

        if (!m_shareService->addSharedFolder(directory).has_value()) {
            showServiceMessage(QStringLiteral("无法添加共享目录，请确认路径有效。"));
        }
    });
}

void SharePage::setShareService(ShareService* service)
{
    if (m_shareService == service) {
        return;
    }
    if (m_shareService) {
        disconnect(m_shareService, nullptr, this, nullptr);
    }

    m_shareService = service;
    if (m_shareService) {
        connect(m_shareService,
                &ShareService::sharedFoldersChanged,
                this,
                &SharePage::renderMyShares);
    }
    renderMyShares();
}

void SharePage::renderMyShares()
{
    if (!m_myShareList) {
        return;
    }

    m_myShareList->clear();
    m_sharedFolders = m_shareService
        ? m_shareService->sharedFolders()
        : QList<SharedFolder>();

    if (m_sharedFolders.isEmpty()) {
        auto* emptyItem = new QListWidgetItem(QStringLiteral("暂无共享目录"));
        emptyItem->setFlags(Qt::ItemIsEnabled);
        emptyItem->setSizeHint(QSize(0, 52));
        m_myShareList->addItem(emptyItem);
        return;
    }

    for (const SharedFolder& folder : m_sharedFolders) {
        auto* item = new QListWidgetItem();
        item->setData(Qt::UserRole, folder.shareId);
        item->setSizeHint(QSize(0, 72));
        m_myShareList->addItem(item);
        m_myShareList->setItemWidget(item, createShareRow(folder));
    }
}

QWidget* SharePage::createShareRow(const SharedFolder& folder)
{
    auto* row = new QWidget(m_myShareList);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(12);

    auto* textContainer = new QWidget(row);
    auto* textLayout = new QVBoxLayout(textContainer);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(4);

    auto* nameLabel = new QLabel(folder.displayName, textContainer);
    nameLabel->setObjectName(QStringLiteral("shareName_%1").arg(folder.shareId));
    nameLabel->setStyleSheet(UiStyle::sectionTitleStyle());

    auto* pathLabel = new QLabel(QDir::toNativeSeparators(folder.localPath), textContainer);
    pathLabel->setObjectName(QStringLiteral("sharePath_%1").arg(folder.shareId));
    pathLabel->setStyleSheet(UiStyle::mutedTextStyle());
    pathLabel->setTextFormat(Qt::PlainText);

    textLayout->addWidget(nameLabel);
    textLayout->addWidget(pathLabel);

    auto* activeCheck = new QCheckBox(QStringLiteral("启用"), row);
    activeCheck->setObjectName(QStringLiteral("shareActiveCheck_%1").arg(folder.shareId));
    activeCheck->setChecked(folder.isActive);
    activeCheck->setEnabled(m_shareService != nullptr);

    auto* removeButton = new QPushButton(QStringLiteral("删除"), row);
    removeButton->setObjectName(QStringLiteral("shareRemoveButton_%1").arg(folder.shareId));
    removeButton->setStyleSheet(UiStyle::secondaryButtonStyle());
    removeButton->setEnabled(m_shareService != nullptr);

    connect(activeCheck, &QCheckBox::toggled, this, [this, folder](bool checked) {
        if (m_shareService) {
            m_shareService->setSharedFolderActive(folder.shareId, checked);
        }
    });
    connect(removeButton, &QPushButton::clicked, this, [this, folder]() {
        if (m_shareService) {
            m_shareService->removeSharedFolder(folder.shareId);
        }
    });

    layout->addWidget(textContainer, 1);
    layout->addWidget(activeCheck);
    layout->addWidget(removeButton);
    return row;
}

void SharePage::showServiceMessage(const QString& message)
{
    if (!m_serviceHint) {
        return;
    }
    m_serviceHint->setText(message);
    m_serviceHint->setVisible(true);
}

} // namespace FengSui
