// ContactsPage.cpp
// 联系人页实现：实时展示 BeaconService 发现到的在线设备，支持手动添加 IP。

#include "ui/contacts/ContactsPage.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QSize>
#include <QVBoxLayout>
#include <QWidget>

#include "core/BeaconService.h"
#include "storage/ManualPeerRepository.h"
#include "ui/UiStyle.h"
#include "ui/contacts/AddPeerDialog.h"

namespace FengSui {

namespace {

// 固定行高避免设备信息刷新时列表抖动。
constexpr int kPeerRowHeight = 76;

} // namespace

ContactsPage::ContactsPage(BeaconService* beaconService,
                           NetworkPolicy* networkPolicy,
                           ManualPeerRepository* manualPeerRepository,
                           QWidget* parent)
    : QWidget(parent)
    , m_beaconService(beaconService)
    , m_networkPolicy(networkPolicy)
    , m_manualPeerRepository(manualPeerRepository)
{
    setupUi();
    connectBeaconService();
    updateEmptyState();
}

void ContactsPage::onPeerOnline(PeerInfo peer)
{
    if (peer.peerId.trimmed().isEmpty() || !peer.online) {
        return;
    }
    if (!peer.peerId.startsWith(QStringLiteral("manual:"))
        && hasManualPeerAtEndpoint(peer.ip, peer.port)) {
        qInfo() << "Ignoring auto-discovered peer because manual peer owns endpoint:"
                << peer.ip << peer.port;
        return;
    }

    upsertPeer(peer);
}

void ContactsPage::onPeerOffline(QString peerId)
{
    removePeer(peerId);
}

void ContactsPage::onItemDoubleClicked(QListWidgetItem* item)
{
    if (!item) {
        return;
    }

    const QString peerId = item->data(Qt::UserRole).toString();
    if (!m_onlinePeers.contains(peerId)) {
        return;
    }

    emit peerActivated(m_onlinePeers.value(peerId));
}

void ContactsPage::setupUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);

    // 标题栏：标题 + 添加设备按钮
    auto* header = new QWidget(this);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(12);

    auto* titleBlock = new QWidget(header);
    auto* titleLayout = new QVBoxLayout(titleBlock);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(2);

    auto* titleLabel = new QLabel(QStringLiteral("联系人"), titleBlock);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));
    titleLabel->setStyleSheet(UiStyle::pageTitleStyle());

    auto* subtitleLabel = new QLabel(QStringLiteral("自动发现和手动添加的局域网设备"), titleBlock);
    subtitleLabel->setStyleSheet(UiStyle::mutedTextStyle());
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(subtitleLabel);

    m_addBtn = new QPushButton(QStringLiteral("+ 添加设备"), header);
    m_addBtn->setObjectName(QStringLiteral("addPeerButton"));
    m_addBtn->setFixedHeight(32);
    m_addBtn->setStyleSheet(UiStyle::primaryButtonStyle());

    headerLayout->addWidget(titleBlock, 1);
    headerLayout->addWidget(m_addBtn);

    m_peerList = new QListWidget(this);
    m_peerList->setObjectName(QStringLiteral("contactsPeerList"));
    m_peerList->setFocusPolicy(Qt::NoFocus);
    m_peerList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_peerList->setStyleSheet(
        "QListWidget {"
        "  border: 1px solid #d8dee8; border-radius: 8px;"
        "  background-color: #ffffff;"
        "}"
        "QListWidget::item {"
        "  border-bottom: 1px solid #eef1f5;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #e7f5f4;"
        "}"
    );
    connect(m_peerList,
            &QListWidget::itemDoubleClicked,
            this,
            &ContactsPage::onItemDoubleClicked);

    m_emptyLabel = new QLabel(QStringLiteral("暂无在线设备"), this);
    m_emptyLabel->setObjectName(QStringLiteral("contactsEmptyLabel"));
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet(
        "color: #667085; font-size: 14px; background: #ffffff;"
        "border: 1px dashed #cfd7e3; border-radius: 8px;");

    layout->addWidget(header);
    layout->addWidget(m_peerList, 1);
    layout->addWidget(m_emptyLabel, 1);

    connect(m_addBtn, &QPushButton::clicked, this, &ContactsPage::onAddClicked);
}

void ContactsPage::connectBeaconService()
{
    if (!m_beaconService) {
        return;
    }

    connect(m_beaconService, &BeaconService::peerOnline, this, &ContactsPage::onPeerOnline);
    connect(m_beaconService, &BeaconService::peerOffline, this, &ContactsPage::onPeerOffline);

    // 先加载当前快照，避免页面创建前已经发现的设备无法显示。
    const QList<PeerInfo> peers = m_beaconService->peers();
    for (const PeerInfo& peer : peers) {
        if (peer.online) {
            upsertPeer(peer);
        }
    }
}

void ContactsPage::upsertPeer(const PeerInfo& peer)
{
    m_onlinePeers.insert(peer.peerId, peer);

    QListWidgetItem* item = nullptr;
    const QList<QListWidgetItem*> matches = m_peerList->findItems(peer.peerId, Qt::MatchExactly);
    if (!matches.isEmpty()) {
        item = matches.first();
    } else {
        item = new QListWidgetItem();
        item->setSizeHint(QSize(0, kPeerRowHeight));
        item->setText(peer.peerId);
        item->setData(Qt::UserRole, peer.peerId);
        m_peerList->addItem(item);
    }

    m_peerList->setItemWidget(item, createPeerRow(peer));
    updateEmptyState();
}

void ContactsPage::removePeer(const QString& peerId)
{
    m_onlinePeers.remove(peerId);

    const QList<QListWidgetItem*> matches = m_peerList->findItems(peerId, Qt::MatchExactly);
    for (QListWidgetItem* item : matches) {
        delete m_peerList->takeItem(m_peerList->row(item));
    }

    updateEmptyState();
}

void ContactsPage::updateEmptyState()
{
    const bool hasPeers = !m_onlinePeers.isEmpty();
    m_peerList->setVisible(hasPeers);
    m_emptyLabel->setVisible(!hasPeers);
}

QWidget* ContactsPage::createPeerRow(const PeerInfo& peer)
{
    auto* row = new QWidget(m_peerList);
    auto* rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(16, 8, 16, 8);
    rowLayout->setSpacing(12);

    auto* osBadgeLabel = new QLabel(osBadgeText(peer.os), row);
    osBadgeLabel->setFixedWidth(54);
    osBadgeLabel->setAlignment(Qt::AlignCenter);
    osBadgeLabel->setStyleSheet(
        "background-color: #eef4ff;"
        "border-radius: 4px;"
        "color: #175cd3;"
        "font-size: 12px;"
        "font-weight: bold;"
        "padding: 4px;"
    );

    auto* textContainer = new QWidget(row);
    auto* textLayout = new QVBoxLayout(textContainer);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);

    auto* nameLabel = new QLabel(peer.displayName, textContainer);
    nameLabel->setStyleSheet("font-size: 14px; font-weight: 700; color: #101828;");

    const QString detailText = QStringLiteral("%1  %2")
                                   .arg(peer.deviceName, endpointText(peer));
    auto* detailLabel = new QLabel(detailText, textContainer);
    detailLabel->setStyleSheet("font-size: 12px; color: #667085;");

    textLayout->addWidget(nameLabel);
    textLayout->addWidget(detailLabel);

    auto* statusLabel = new QLabel(QStringLiteral("在线"), row);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet(UiStyle::pillStyle(QStringLiteral("#e7f5f4"),
                                                  QStringLiteral("#0f6f70")));

    rowLayout->addWidget(osBadgeLabel);
    rowLayout->addWidget(textContainer, 1);
    rowLayout->addWidget(statusLabel);

    return row;
}

QString ContactsPage::osBadgeText(const QString& os) const
{
    const QString normalized = os.trimmed().toLower();
    if (normalized == "windows") {
        return QStringLiteral("Win");
    }
    if (normalized == "linux") {
        return QStringLiteral("Linux");
    }
    if (normalized == "macos") {
        return QStringLiteral("macOS");
    }

    return QStringLiteral("OS");
}

QString ContactsPage::endpointText(const PeerInfo& peer) const
{
    if (peer.ip.trimmed().isEmpty() || peer.port == 0) {
        return QStringLiteral("-");
    }

    return QStringLiteral("%1:%2").arg(peer.ip).arg(peer.port);
}

bool ContactsPage::hasPeerAtEndpoint(const QString& ip, quint16 port) const
{
    for (const PeerInfo& peer : m_onlinePeers) {
        if (peer.ip == ip && peer.port == port) {
            return true;
        }
    }
    return false;
}

bool ContactsPage::hasManualPeerAtEndpoint(const QString& ip, quint16 port) const
{
    for (const PeerInfo& peer : m_onlinePeers) {
        if (peer.peerId.startsWith(QStringLiteral("manual:"))
            && peer.ip == ip
            && peer.port == port) {
            return true;
        }
    }
    return false;
}

void ContactsPage::persistManualPeer(const PeerInfo& peer)
{
    if (!m_manualPeerRepository
        || !peer.peerId.startsWith(QStringLiteral("manual:"))) {
        return;
    }

    ManualPeer manualPeer;
    manualPeer.name = peer.displayName;
    manualPeer.host = peer.ip;
    manualPeer.port = peer.port;
    manualPeer.lastSuccessAt = peer.lastSeenAt.isValid()
        ? peer.lastSeenAt
        : QDateTime::currentDateTimeUtc();
    m_manualPeerRepository->upsertManualPeer(manualPeer);
}

void ContactsPage::onAddClicked()
{
    constexpr quint16 kDefaultTcpPort = 8787;
    AddPeerDialog dialog(kDefaultTcpPort, m_networkPolicy, this);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    PeerInfo peer = dialog.resultPeer();

    // 检查是否已存在相同 IP:端口
    if (hasPeerAtEndpoint(peer.ip, peer.port)) {
        QMessageBox::information(this,
                                 QStringLiteral("添加设备"),
                                 QStringLiteral("该设备已在列表中"));
        return;
    }

    addManualPeerSlot(peer);
}

void ContactsPage::addManualPeerSlot(PeerInfo peer)
{
    peer.online = true;
    peer.lastSeenAt = QDateTime::currentDateTimeUtc();
    persistManualPeer(peer);
    upsertPeer(peer);
}

} // namespace FengSui
