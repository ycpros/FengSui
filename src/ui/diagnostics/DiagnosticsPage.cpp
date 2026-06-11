// DiagnosticsPage.cpp
// Network diagnostics shell backed by currently available settings/services.

#include "ui/diagnostics/DiagnosticsPage.h"

#include "app/AppSettings.h"
#include "core/BeaconService.h"
#include "models/NetworkPolicy.h"
#include "platform/InterfaceEnumerator.h"
#include "ui/UiStyle.h"

#include <QAbstractItemView>
#include <QColor>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace FengSui
{

namespace {

QTableWidgetItem* readOnlyItem(const QString& text)
{
    auto* item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

QString typeText(InterfaceType type)
{
    switch (type) {
    case InterfaceType::Physical:
        return QStringLiteral("物理");
    case InterfaceType::Virtual:
        return QStringLiteral("虚拟");
    case InterfaceType::Loopback:
        return QStringLiteral("回环");
    case InterfaceType::LinkLocal:
        return QStringLiteral("链路本地");
    case InterfaceType::Public:
        return QStringLiteral("公网");
    case InterfaceType::Unknown:
    default:
        return QStringLiteral("未知");
    }
}

QString modeDisplayText(const QString& mode)
{
    if (mode == QStringLiteral("multi_lan")) {
        return QStringLiteral("多网卡内网");
    }
    if (mode == QStringLiteral("compat_test")) {
        return QStringLiteral("兼容测试");
    }
    return QStringLiteral("安全内网");
}

} // namespace

DiagnosticsPage::DiagnosticsPage(AppSettings* settings,
                                 BeaconService* beaconService,
                                 NetworkPolicy* networkPolicy,
                                 QWidget* parent)
    : QWidget(parent)
    , m_settings(settings)
    , m_beaconService(beaconService)
    , m_networkPolicy(networkPolicy)
{
    setupUi();
}

void DiagnosticsPage::setupUi()
{
    setObjectName(QStringLiteral("diagnosticsPage"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(14);

    auto* headerLayout = new QHBoxLayout();
    auto* titleLabel = new QLabel(QStringLiteral("网络诊断"), this);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));
    titleLabel->setStyleSheet(UiStyle::pageTitleStyle());

    auto* refreshButton = new QPushButton(QStringLiteral("重新检测"), this);
    refreshButton->setObjectName(QStringLiteral("diagnosticsRefreshButton"));
    refreshButton->setStyleSheet(UiStyle::secondaryButtonStyle());

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(refreshButton);

    auto* summaryLayout = new QHBoxLayout();
    summaryLayout->setSpacing(8);

    m_modeLabel = new QLabel(this);
    m_modeLabel->setObjectName(QStringLiteral("diagnosticsModeLabel"));
    m_portLabel = new QLabel(this);
    m_portLabel->setObjectName(QStringLiteral("diagnosticsPortLabel"));
    m_cidrsLabel = new QLabel(this);
    m_cidrsLabel->setObjectName(QStringLiteral("diagnosticsCidrsLabel"));
    m_onlinePeersLabel = new QLabel(this);
    m_onlinePeersLabel->setObjectName(QStringLiteral("diagnosticsOnlinePeersLabel"));

    for (QLabel* label : {m_modeLabel, m_portLabel, m_cidrsLabel, m_onlinePeersLabel}) {
        label->setStyleSheet(UiStyle::pillStyle(QStringLiteral("#e7f5f4"),
                                                QStringLiteral("#0f6f70")));
        summaryLayout->addWidget(label);
    }
    summaryLayout->addStretch();

    auto* ifaceTitle = new QLabel(QStringLiteral("本机 IPv4 网卡"), this);
    ifaceTitle->setStyleSheet(UiStyle::sectionTitleStyle());

    m_interfaceTable = new QTableWidget(this);
    m_interfaceTable->setObjectName(QStringLiteral("diagnosticsInterfaceTable"));
    m_interfaceTable->setColumnCount(4);
    m_interfaceTable->setHorizontalHeaderLabels({
        QStringLiteral("名称"),
        QStringLiteral("IP"),
        QStringLiteral("CIDR"),
        QStringLiteral("类型"),
    });
    m_interfaceTable->verticalHeader()->setVisible(false);
    m_interfaceTable->horizontalHeader()->setStretchLastSection(true);
    m_interfaceTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_interfaceTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_interfaceTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_interfaceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_interfaceTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_interfaceTable->setMinimumHeight(150);

    auto* checksTitle = new QLabel(QStringLiteral("检测项"), this);
    checksTitle->setStyleSheet(UiStyle::sectionTitleStyle());

    m_checkTable = new QTableWidget(this);
    m_checkTable->setObjectName(QStringLiteral("diagnosticsCheckTable"));
    m_checkTable->setColumnCount(3);
    m_checkTable->setHorizontalHeaderLabels({
        QStringLiteral("项目"),
        QStringLiteral("状态"),
        QStringLiteral("说明"),
    });
    m_checkTable->verticalHeader()->setVisible(false);
    m_checkTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_checkTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_checkTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_checkTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_checkTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_checkTable->setMinimumHeight(170);

    layout->addLayout(headerLayout);
    layout->addLayout(summaryLayout);
    layout->addWidget(ifaceTitle);
    layout->addWidget(m_interfaceTable);
    layout->addWidget(checksTitle);
    layout->addWidget(m_checkTable);
    layout->addStretch();

    connect(refreshButton, &QPushButton::clicked, this, &DiagnosticsPage::refresh);

    refresh();
}

void DiagnosticsPage::refresh()
{
    const QString mode = m_settings ? m_settings->networkMode() : QStringLiteral("secure_lan");
    const quint16 port = m_settings ? m_settings->listenPort() : 8787;
    QStringList cidrs = m_networkPolicy ? m_networkPolicy->allowedCidrs() : QStringList();
    if (cidrs.isEmpty() && m_settings) {
        cidrs = m_settings->allowedCidrs().split(QStringLiteral(","), Qt::SkipEmptyParts);
    }

    int onlineCount = 0;
    if (m_beaconService) {
        const QList<PeerInfo> peers = m_beaconService->peers();
        for (const PeerInfo& peer : peers) {
            if (peer.online) {
                ++onlineCount;
            }
        }
    }

    if (m_modeLabel) {
        m_modeLabel->setText(QStringLiteral("模式 %1").arg(modeDisplayText(mode)));
    }
    if (m_portLabel) {
        m_portLabel->setText(QStringLiteral("端口 %1").arg(port));
    }
    if (m_cidrsLabel) {
        m_cidrsLabel->setText(cidrs.isEmpty()
                                  ? QStringLiteral("允许网段 未配置")
                                  : QStringLiteral("允许网段 %1").arg(cidrs.join(QStringLiteral(", "))));
    }
    if (m_onlinePeersLabel) {
        m_onlinePeersLabel->setText(QStringLiteral("在线设备 %1").arg(onlineCount));
    }

    renderInterfaces();
    renderChecks();
}

void DiagnosticsPage::renderInterfaces()
{
    if (!m_interfaceTable) {
        return;
    }

    const QList<NetworkInterfaceInfo> interfaces = InterfaceEnumerator::enumerate();
    m_interfaceTable->setRowCount(interfaces.size());
    for (int row = 0; row < interfaces.size(); ++row) {
        const NetworkInterfaceInfo& iface = interfaces.at(row);
        m_interfaceTable->setRowHeight(row, 38);
        m_interfaceTable->setItem(row, 0, readOnlyItem(iface.name));
        m_interfaceTable->setItem(row, 1, readOnlyItem(iface.ip.toString()));
        m_interfaceTable->setItem(row, 2, readOnlyItem(iface.cidr()));
        m_interfaceTable->setItem(row, 3, readOnlyItem(typeText(iface.type)));
    }
}

void DiagnosticsPage::renderChecks()
{
    if (!m_checkTable) {
        return;
    }

    m_checkTable->setRowCount(5);
    setCheckRow(0,
                QStringLiteral("TCP 监听端口"),
                m_settings ? QStringLiteral("可读取") : QStringLiteral("未连接"),
                m_settings
                    ? QStringLiteral("当前配置端口为 %1，真实监听探测将在 DiagnosticsService 接入。")
                          .arg(m_settings->listenPort())
                    : QStringLiteral("缺少设置实例，无法读取端口。"));
    setCheckRow(1,
                QStringLiteral("UDP 广播测试"),
                QStringLiteral("未实现"),
                QStringLiteral("当前 UI 仅展示检测入口，尚未发送测试广播包。"));
    setCheckRow(2,
                QStringLiteral("防火墙建议"),
                QStringLiteral("未实现"),
                QStringLiteral("Windows Defender 规则检测将在平台诊断服务接入。"));
    setCheckRow(3,
                QStringLiteral("授权网段"),
                (m_networkPolicy && !m_networkPolicy->allowedCidrs().isEmpty())
                    ? QStringLiteral("已配置")
                    : QStringLiteral("未配置"),
                (m_networkPolicy && !m_networkPolicy->allowedCidrs().isEmpty())
                    ? m_networkPolicy->allowedCidrs().join(QStringLiteral(", "))
                    : QStringLiteral("未读取到运行时允许 CIDR。"));
    setCheckRow(4,
                QStringLiteral("共享服务"),
                QStringLiteral("未接入"),
                QStringLiteral("共享浏览和授权协议不在本轮 GUI 壳层范围内。"));
}

void DiagnosticsPage::setCheckRow(int row,
                                  const QString& item,
                                  const QString& status,
                                  const QString& detail)
{
    m_checkTable->setRowHeight(row, 38);
    auto* statusItem = readOnlyItem(status);
    if (status == QStringLiteral("已配置") || status == QStringLiteral("可读取")) {
        statusItem->setForeground(QColor(QStringLiteral("#187a3b")));
    } else if (status == QStringLiteral("未实现") || status == QStringLiteral("未接入")) {
        statusItem->setForeground(QColor(QStringLiteral("#8a5a00")));
    } else {
        statusItem->setForeground(QColor(QStringLiteral("#b42318")));
    }

    m_checkTable->setItem(row, 0, readOnlyItem(item));
    m_checkTable->setItem(row, 1, statusItem);
    m_checkTable->setItem(row, 2, readOnlyItem(detail));
}

} // namespace FengSui
