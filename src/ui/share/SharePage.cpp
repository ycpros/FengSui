// SharePage.cpp
// 共享文件页面实现：远程共享浏览、文件下载、本机共享管理和访问授权弹窗。

#include "ui/share/SharePage.h"

#include "core/BeaconService.h"
#include "core/ShareService.h"
#include "ui/UiStyle.h"
#include "ui/share/ShareAccessDialog.h"

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

QString parentRemotePath(const QString& path)
{
    QString normalized = path.trimmed();
    normalized.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (normalized.isEmpty() || normalized == QStringLiteral("/")) {
        return QStringLiteral("/");
    }
    if (!normalized.startsWith(QLatin1Char('/'))) {
        normalized.prepend(QLatin1Char('/'));
    }
    const int slash = normalized.lastIndexOf(QLatin1Char('/'));
    if (slash <= 0) {
        return QStringLiteral("/");
    }
    return normalized.left(slash);
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
        QStringLiteral("浏览同网段设备的共享目录，或管理本机对外提供的共享。"),
        this);
    subtitleLabel->setObjectName(QStringLiteral("pageSubtitle"));
    subtitleLabel->setStyleSheet(UiStyle::mutedTextStyle());

    auto* tabWidget = new QTabWidget(this);
    tabWidget->setObjectName(QStringLiteral("shareTabWidget"));

    auto* browseTab = new QWidget(tabWidget);
    auto* browseLayout = new QVBoxLayout(browseTab);
    browseLayout->setContentsMargins(16, 16, 16, 16);
    browseLayout->setSpacing(12);

    m_browseHint = new QLabel(QStringLiteral("请选择在线共享源。"), browseTab);
    m_browseHint->setObjectName(QStringLiteral("shareBrowseHintLabel"));
    m_browseHint->setStyleSheet(UiStyle::pillStyle(QStringLiteral("#eef4ff"),
                                                   QStringLiteral("#175cd3")));

    auto* splitter = new QSplitter(Qt::Horizontal, browseTab);
    splitter->setHandleWidth(1);

    auto* sourcePanel = new QWidget(splitter);
    auto* sourceLayout = new QVBoxLayout(sourcePanel);
    sourceLayout->setContentsMargins(0, 0, 12, 0);
    sourceLayout->setSpacing(8);
    auto* sourceTitle = new QLabel(QStringLiteral("在线共享源"), sourcePanel);
    sourceTitle->setStyleSheet(UiStyle::sectionTitleStyle());
    m_sourceList = new QListWidget(sourcePanel);
    m_sourceList->setObjectName(QStringLiteral("shareSourceList"));
    auto* shareTitle = new QLabel(QStringLiteral("共享目录"), sourcePanel);
    shareTitle->setStyleSheet(UiStyle::sectionTitleStyle());
    m_remoteShareList = new QListWidget(sourcePanel);
    m_remoteShareList->setObjectName(QStringLiteral("remoteShareList"));
    sourceLayout->addWidget(sourceTitle);
    sourceLayout->addWidget(m_sourceList, 1);
    sourceLayout->addWidget(shareTitle);
    sourceLayout->addWidget(m_remoteShareList, 1);

    auto* browserPanel = new QWidget(splitter);
    auto* browserLayout = new QVBoxLayout(browserPanel);
    browserLayout->setContentsMargins(12, 0, 0, 0);
    browserLayout->setSpacing(8);
    m_breadcrumbLabel = new QLabel(QStringLiteral("位置：/"), browserPanel);
    m_breadcrumbLabel->setObjectName(QStringLiteral("shareBreadcrumbLabel"));
    m_breadcrumbLabel->setStyleSheet(UiStyle::mutedTextStyle());
    m_fileTable = new QTableWidget(browserPanel);
    m_fileTable->setObjectName(QStringLiteral("shareFileTable"));
    m_fileTable->setColumnCount(4);
    m_fileTable->setHorizontalHeaderLabels({
        QStringLiteral("名称"),
        QStringLiteral("类型"),
        QStringLiteral("大小"),
        QStringLiteral("修改时间"),
    });
    m_fileTable->verticalHeader()->setVisible(false);
    m_fileTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_fileTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_fileTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_fileTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_fileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_fileTable->setSelectionMode(QAbstractItemView::SingleSelection);
    browserLayout->addWidget(m_breadcrumbLabel);
    browserLayout->addWidget(m_fileTable, 1);

    splitter->addWidget(sourcePanel);
    splitter->addWidget(browserPanel);
    splitter->setSizes({300, 620});

    browseLayout->addWidget(m_browseHint);
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

    tabWidget->addTab(browseTab, QStringLiteral("浏览共享"));
    tabWidget->addTab(myTab, QStringLiteral("我的共享"));

    myLayout->addLayout(toolbar);
    myLayout->addWidget(m_serviceHint);
    myLayout->addWidget(m_myShareList, 1);

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

    connect(m_sourceList, &QListWidget::itemActivated,
            this, [this](QListWidgetItem*) { requestSelectedSourceShares(); });
    connect(m_sourceList, &QListWidget::currentItemChanged,
            this, [this](QListWidgetItem*, QListWidgetItem*) {
                requestSelectedSourceShares();
            });
    connect(m_remoteShareList, &QListWidget::itemActivated,
            this, [this](QListWidgetItem*) { requestSelectedShareItems(); });
    connect(m_remoteShareList, &QListWidget::currentItemChanged,
            this, [this](QListWidgetItem*, QListWidgetItem*) {
                requestSelectedShareItems();
            });
    connect(m_fileTable, &QTableWidget::cellDoubleClicked,
            this, [this](int row, int) { openSelectedRemoteItem(row); });

    renderShareSources();
    renderRemoteItems(QString(), QString(), QStringLiteral("/"), {});
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
        connect(m_shareService,
                &ShareService::remoteShareListReceived,
                this,
                [this](const QString& peerId,
                       const QString&,
                       const QList<RemoteSharedFolder>& shares) {
                    renderRemoteShares(peerId, shares);
                });
        connect(m_shareService,
                &ShareService::remoteShareItemsReceived,
                this,
                [this](const QString& peerId,
                       const QString&,
                       const QString& shareId,
                       const QString& path,
                       const QList<RemoteShareItem>& items) {
                    renderRemoteItems(peerId, shareId, path, items);
                });
        connect(m_shareService,
                &ShareService::remoteShareDownloadStarted,
                this,
                [this](const QString&, const QString& fileName, qint64, const QString&) {
                    showBrowseMessage(QStringLiteral("开始下载：%1").arg(fileName));
                });
        connect(m_shareService,
                &ShareService::remoteShareDownloadCompleted,
                this,
                [this](const QString&, const QString& localPath) {
                    showBrowseMessage(QStringLiteral("下载完成：%1").arg(localPath));
                });
        connect(m_shareService,
                &ShareService::remoteShareFailed,
                this,
                [this](const QString&, const QString& errorMessage) {
                    showBrowseMessage(errorMessage);
                });
        connect(m_shareService,
                &ShareService::accessRequested,
                this,
                [this](const QString& requestId,
                       const QString& requesterName,
                       const QString& deviceName,
                       const QString& shareName) {
                    ShareAccessDialog dialog(requesterName, deviceName, shareName, this);
                    const bool allowed = dialog.exec() == QDialog::Accepted;
                    m_shareService->resolveAccessRequest(requestId,
                                                         allowed,
                                                         dialog.rememberChoice());
                });
    }
    renderMyShares();
}

void SharePage::setBeaconService(BeaconService* service)
{
    if (m_beaconService == service) {
        return;
    }
    if (m_beaconService) {
        disconnect(m_beaconService, nullptr, this, nullptr);
    }

    m_beaconService = service;
    if (m_beaconService) {
        connect(m_beaconService, &BeaconService::peerOnline,
                this, [this](PeerInfo) { renderShareSources(); });
        connect(m_beaconService, &BeaconService::peerOffline,
                this, [this](QString) { renderShareSources(); });
    }
    renderShareSources();
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

void SharePage::renderShareSources()
{
    if (!m_sourceList) {
        return;
    }

    const QString previousPeerId = m_currentPeerId;
    m_sharePeers.clear();
    m_sourceList->clear();

    const QList<PeerInfo> peers = m_beaconService ? m_beaconService->peers() : QList<PeerInfo>();
    for (const PeerInfo& peer : peers) {
        if (!peer.online || !peer.shareEnabled) {
            continue;
        }
        m_sharePeers.insert(peer.peerId, peer);
        auto* item = new QListWidgetItem(
            QStringLiteral("%1  %2").arg(peer.displayName, peer.ip));
        item->setData(Qt::UserRole, peer.peerId);
        item->setSizeHint(QSize(0, 44));
        m_sourceList->addItem(item);
        if (peer.peerId == previousPeerId) {
            m_sourceList->setCurrentItem(item);
        }
    }

    if (m_sourceList->count() == 0) {
        auto* emptyItem = new QListWidgetItem(QStringLiteral("暂无在线共享源"));
        emptyItem->setFlags(Qt::ItemIsEnabled);
        m_sourceList->addItem(emptyItem);
        m_currentPeerId.clear();
        m_remoteShareList->clear();
        renderRemoteItems(QString(), QString(), QStringLiteral("/"), {});
    }
}

void SharePage::renderRemoteShares(const QString& peerId,
                                   const QList<RemoteSharedFolder>& shares)
{
    if (!m_remoteShareList || peerId != m_currentPeerId) {
        return;
    }

    m_remoteSharesByPeer.insert(peerId, shares);
    m_currentRemoteShares = shares;
    m_remoteShareList->clear();

    if (shares.isEmpty()) {
        auto* emptyItem = new QListWidgetItem(QStringLiteral("该设备暂无可访问共享"));
        emptyItem->setFlags(Qt::ItemIsEnabled);
        m_remoteShareList->addItem(emptyItem);
        renderRemoteItems(peerId, QString(), QStringLiteral("/"), {});
        return;
    }

    for (const RemoteSharedFolder& folder : shares) {
        auto* item = new QListWidgetItem(
            QStringLiteral("%1（%2 个文件）")
                .arg(folder.displayName)
                .arg(folder.fileCount));
        item->setData(Qt::UserRole, folder.shareId);
        item->setSizeHint(QSize(0, 44));
        m_remoteShareList->addItem(item);
    }
    m_remoteShareList->setCurrentRow(0);
}

void SharePage::renderRemoteItems(const QString& peerId,
                                  const QString& shareId,
                                  const QString& path,
                                  const QList<RemoteShareItem>& items)
{
    if (!m_fileTable || (!peerId.isEmpty() && peerId != m_currentPeerId)) {
        return;
    }

    m_currentShareId = shareId;
    m_currentPath = path.trimmed().isEmpty() ? QStringLiteral("/") : path;
    m_currentRemoteItems = items;
    if (m_breadcrumbLabel) {
        m_breadcrumbLabel->setText(QStringLiteral("位置：%1").arg(m_currentPath));
    }

    m_fileTable->setRowCount(0);
    int row = 0;
    if (!shareId.isEmpty() && m_currentPath != QStringLiteral("/")) {
        m_fileTable->insertRow(row);
        auto* name = readOnlyItem(QStringLiteral(".."));
        name->setData(Qt::UserRole, parentRemotePath(m_currentPath));
        name->setData(Qt::UserRole + 1, true);
        m_fileTable->setItem(row, 0, name);
        m_fileTable->setItem(row, 1, readOnlyItem(QStringLiteral("目录")));
        m_fileTable->setItem(row, 2, readOnlyItem(QStringLiteral("-")));
        m_fileTable->setItem(row, 3, readOnlyItem(QStringLiteral("-")));
        ++row;
    }

    for (const RemoteShareItem& item : items) {
        m_fileTable->insertRow(row);
        auto* name = readOnlyItem(item.name);
        name->setData(Qt::UserRole, item.path);
        name->setData(Qt::UserRole + 1, item.isDir);
        m_fileTable->setItem(row, 0, name);
        m_fileTable->setItem(row, 1, readOnlyItem(item.isDir
                                                  ? QStringLiteral("目录")
                                                  : QStringLiteral("文件")));
        m_fileTable->setItem(row, 2, readOnlyItem(item.isDir
                                                  ? QStringLiteral("-")
                                                  : QString::number(item.size)));
        m_fileTable->setItem(row, 3, readOnlyItem(item.modifiedAt));
        ++row;
    }

    if (row == 0) {
        m_fileTable->insertRow(0);
        m_fileTable->setItem(0, 0, readOnlyItem(QStringLiteral("选择共享目录后显示内容")));
        m_fileTable->setItem(0, 1, readOnlyItem(QStringLiteral("-")));
        m_fileTable->setItem(0, 2, readOnlyItem(QStringLiteral("-")));
        m_fileTable->setItem(0, 3, readOnlyItem(QStringLiteral("-")));
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

void SharePage::showBrowseMessage(const QString& message)
{
    if (!m_browseHint) {
        return;
    }
    m_browseHint->setText(message);
    m_browseHint->setVisible(true);
}

void SharePage::requestSelectedSourceShares()
{
    if (!m_shareService || !m_sourceList || !m_sourceList->currentItem()) {
        return;
    }

    const QString peerId = m_sourceList->currentItem()->data(Qt::UserRole).toString();
    if (!m_sharePeers.contains(peerId)) {
        return;
    }
    m_currentPeerId = peerId;
    m_remoteShareList->clear();
    renderRemoteItems(peerId, QString(), QStringLiteral("/"), {});
    showBrowseMessage(QStringLiteral("正在读取共享目录..."));
    m_shareService->requestShareList(m_sharePeers.value(peerId));
}

void SharePage::requestSelectedShareItems()
{
    if (!m_shareService || !m_remoteShareList || !m_remoteShareList->currentItem()) {
        return;
    }
    const QString shareId = m_remoteShareList->currentItem()->data(Qt::UserRole).toString();
    if (shareId.trimmed().isEmpty() || !m_sharePeers.contains(m_currentPeerId)) {
        return;
    }

    m_currentShareId = shareId;
    m_currentPath = QStringLiteral("/");
    showBrowseMessage(QStringLiteral("正在读取目录..."));
    m_shareService->requestShareItems(m_sharePeers.value(m_currentPeerId),
                                      shareId,
                                      m_currentPath);
}

void SharePage::openSelectedRemoteItem(int row)
{
    if (!m_shareService || !m_fileTable || row < 0 || row >= m_fileTable->rowCount()
        || !m_fileTable->item(row, 0) || !m_sharePeers.contains(m_currentPeerId)
        || m_currentShareId.trimmed().isEmpty()) {
        return;
    }

    const QString path = m_fileTable->item(row, 0)->data(Qt::UserRole).toString();
    const bool isDir = m_fileTable->item(row, 0)->data(Qt::UserRole + 1).toBool();
    if (path.trimmed().isEmpty()) {
        return;
    }

    if (isDir) {
        showBrowseMessage(QStringLiteral("正在读取目录..."));
        m_shareService->requestShareItems(m_sharePeers.value(m_currentPeerId),
                                          m_currentShareId,
                                          path);
        return;
    }

    showBrowseMessage(QStringLiteral("正在请求下载..."));
    m_shareService->requestShareDownload(m_sharePeers.value(m_currentPeerId),
                                         m_currentShareId,
                                         path);
}

PeerInfo SharePage::selectedPeer() const
{
    return m_sharePeers.value(m_currentPeerId);
}

} // namespace FengSui
