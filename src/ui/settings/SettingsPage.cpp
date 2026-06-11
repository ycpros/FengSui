// SettingsPage.cpp
// Settings form shell bound to the existing AppSettings repository.

#include "ui/settings/SettingsPage.h"

#include "app/AppSettings.h"
#include "models/NetworkPolicy.h"
#include "platform/InterfaceEnumerator.h"
#include "storage/ManualPeerRepository.h"
#include "ui/UiStyle.h"
#include "ui/diagnostics/DiagnosticsPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSize>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

namespace FengSui {

namespace {

QStringList splitCsv(const QString& value)
{
    QStringList result;
    const QStringList parts = value.split(QStringLiteral(","), Qt::SkipEmptyParts);
    for (const QString& part : parts) {
        const QString trimmed = part.trimmed();
        if (!trimmed.isEmpty() && !result.contains(trimmed)) {
            result.append(trimmed);
        }
    }
    return result;
}

QString interfaceTypeText(InterfaceType type)
{
    switch (type) {
    case InterfaceType::Physical:
        return QStringLiteral("物理网卡");
    case InterfaceType::Virtual:
        return QStringLiteral("虚拟网卡");
    case InterfaceType::Loopback:
        return QStringLiteral("回环");
    case InterfaceType::LinkLocal:
        return QStringLiteral("链路本地");
    case InterfaceType::Public:
        return QStringLiteral("公网地址");
    case InterfaceType::Unknown:
    default:
        return QStringLiteral("未知");
    }
}

QLabel* sectionLabel(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setStyleSheet(UiStyle::sectionTitleStyle());
    return label;
}

} // namespace

SettingsPage::SettingsPage(AppSettings* settings,
                           NetworkPolicy* networkPolicy,
                           ManualPeerRepository* manualPeerRepository,
                           QWidget* parent)
    : QWidget(parent)
    , m_settings(settings)
    , m_networkPolicy(networkPolicy)
    , m_manualPeerRepository(manualPeerRepository)
{
    setupUi();
}

void SettingsPage::setupUi()
{
    setObjectName(QStringLiteral("settingsPage"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);

    auto* titleLabel = new QLabel(QStringLiteral("设置"), this);
    titleLabel->setObjectName(QStringLiteral("pageTitle"));
    titleLabel->setStyleSheet(UiStyle::pageTitleStyle());

    auto* subtitleLabel = new QLabel(QStringLiteral("管理本机身份、传输路径和局域网发现策略。"), this);
    subtitleLabel->setObjectName(QStringLiteral("pageSubtitle"));
    subtitleLabel->setStyleSheet(UiStyle::mutedTextStyle());

    m_restartHintLabel = new QLabel(QStringLiteral("网络相关设置已变更，重启网络服务后完全生效。"), this);
    m_restartHintLabel->setObjectName(QStringLiteral("settingsRestartHint"));
    m_restartHintLabel->setStyleSheet(UiStyle::pillStyle(QStringLiteral("#fff4d6"),
                                                         QStringLiteral("#8a5a00")));
    m_restartHintLabel->setVisible(false);

    auto* tabWidget = new QTabWidget(this);
    tabWidget->setObjectName(QStringLiteral("settingsTabWidget"));
    tabWidget->addTab(createGeneralTab(), QStringLiteral("常规"));
    tabWidget->addTab(createNetworkTab(), QStringLiteral("网络"));
    tabWidget->addTab(new DiagnosticsPage(m_settings, nullptr, m_networkPolicy, tabWidget),
                      QStringLiteral("网络诊断"));

    layout->addWidget(titleLabel);
    layout->addWidget(subtitleLabel);
    layout->addWidget(m_restartHintLabel);
    layout->addWidget(tabWidget, 1);

    loadSettingsIntoControls();
}

QWidget* SettingsPage::createGeneralTab()
{
    auto* tab = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(tab);
    rootLayout->setContentsMargins(18, 18, 18, 18);
    rootLayout->setSpacing(16);

    auto* formFrame = new QFrame(tab);
    formFrame->setObjectName(QStringLiteral("settingsGeneralFrame"));
    auto* formLayout = new QFormLayout(formFrame);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    formLayout->setHorizontalSpacing(18);
    formLayout->setVerticalSpacing(12);

    m_displayNameEdit = new QLineEdit(formFrame);
    m_displayNameEdit->setObjectName(QStringLiteral("settingsDisplayNameEdit"));
    m_displayNameEdit->setPlaceholderText(QStringLiteral("输入昵称"));

    auto* downloadRow = new QWidget(formFrame);
    auto* downloadLayout = new QHBoxLayout(downloadRow);
    downloadLayout->setContentsMargins(0, 0, 0, 0);
    downloadLayout->setSpacing(8);
    m_downloadDirEdit = new QLineEdit(downloadRow);
    m_downloadDirEdit->setObjectName(QStringLiteral("settingsDownloadDirEdit"));
    auto* browseButton = new QPushButton(QStringLiteral("浏览"), downloadRow);
    browseButton->setObjectName(QStringLiteral("settingsBrowseDownloadButton"));
    downloadLayout->addWidget(m_downloadDirEdit, 1);
    downloadLayout->addWidget(browseButton);

    m_autoStartCheck = new QCheckBox(QStringLiteral("开机启动"), formFrame);
    m_autoStartCheck->setObjectName(QStringLiteral("settingsAutoStartCheck"));

    m_minimizeToTrayCheck = new QCheckBox(QStringLiteral("关闭窗口时最小化到托盘"), formFrame);
    m_minimizeToTrayCheck->setObjectName(QStringLiteral("settingsMinimizeToTrayCheck"));

    formLayout->addRow(QStringLiteral("昵称"), m_displayNameEdit);
    formLayout->addRow(QStringLiteral("下载目录"), downloadRow);
    formLayout->addRow(QStringLiteral("启动行为"), m_autoStartCheck);
    formLayout->addRow(QStringLiteral("托盘"), m_minimizeToTrayCheck);

    auto* noteLabel = new QLabel(
        QStringLiteral("开机启动和托盘行为已写入设置；平台级注册将在托盘增强阶段接入。"),
        tab);
    noteLabel->setWordWrap(true);
    noteLabel->setStyleSheet(UiStyle::mutedTextStyle());

    rootLayout->addWidget(sectionLabel(QStringLiteral("常规"), tab));
    rootLayout->addWidget(formFrame);
    rootLayout->addWidget(noteLabel);
    rootLayout->addStretch();

    connect(m_displayNameEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (!m_loading && m_settings && !text.trimmed().isEmpty()) {
            m_settings->setDisplayName(text.trimmed());
        }
    });
    connect(m_downloadDirEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (!m_loading && m_settings && !text.trimmed().isEmpty()) {
            m_settings->setDownloadDir(text.trimmed());
        }
    });
    connect(m_autoStartCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (!m_loading && m_settings) {
            m_settings->setAutoStart(checked);
        }
    });
    connect(m_minimizeToTrayCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (!m_loading && m_settings) {
            m_settings->setMinimizeToTray(checked);
        }
    });
    connect(browseButton, &QPushButton::clicked, this, [this]() {
        const QString dir = QFileDialog::getExistingDirectory(
            this,
            QStringLiteral("选择下载目录"),
            m_downloadDirEdit ? m_downloadDirEdit->text() : QString());
        if (!dir.isEmpty() && m_downloadDirEdit) {
            m_downloadDirEdit->setText(dir);
        }
    });

    return tab;
}

QWidget* SettingsPage::createNetworkTab()
{
    auto* tab = new QWidget(this);
    auto* scroll = new QScrollArea(tab);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* content = new QWidget(scroll);
    auto* rootLayout = new QVBoxLayout(content);
    rootLayout->setContentsMargins(18, 18, 18, 18);
    rootLayout->setSpacing(16);

    auto* formFrame = new QFrame(content);
    auto* formLayout = new QFormLayout(formFrame);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setHorizontalSpacing(18);
    formLayout->setVerticalSpacing(12);

    m_discoveryCheck = new QCheckBox(QStringLiteral("启用局域网自动发现"), formFrame);
    m_discoveryCheck->setObjectName(QStringLiteral("settingsDiscoveryCheck"));

    m_portSpin = new QSpinBox(formFrame);
    m_portSpin->setObjectName(QStringLiteral("settingsListenPortSpin"));
    m_portSpin->setRange(1, 65535);

    m_networkModeCombo = new QComboBox(formFrame);
    m_networkModeCombo->setObjectName(QStringLiteral("settingsNetworkModeCombo"));
    m_networkModeCombo->addItem(QStringLiteral("安全内网"), QStringLiteral("secure_lan"));
    m_networkModeCombo->addItem(QStringLiteral("多网卡内网"), QStringLiteral("multi_lan"));
    m_networkModeCombo->addItem(QStringLiteral("兼容测试"), QStringLiteral("compat_test"));

    formLayout->addRow(QStringLiteral("发现"), m_discoveryCheck);
    formLayout->addRow(QStringLiteral("监听端口"), m_portSpin);
    formLayout->addRow(QStringLiteral("网络模式"), m_networkModeCombo);

    m_interfaceList = new QListWidget(content);
    m_interfaceList->setObjectName(QStringLiteral("settingsInterfaceList"));
    m_interfaceList->setMinimumHeight(150);

    m_allowedCidrsEdit = new QPlainTextEdit(content);
    m_allowedCidrsEdit->setObjectName(QStringLiteral("settingsAllowedCidrsEdit"));
    m_allowedCidrsEdit->setPlaceholderText(QStringLiteral("192.168.10.0/24"));
    m_allowedCidrsEdit->setMaximumHeight(96);

    m_manualPeerList = new QListWidget(content);
    m_manualPeerList->setObjectName(QStringLiteral("settingsManualPeerList"));
    m_manualPeerList->setMinimumHeight(110);

    auto* compatHint = new QLabel(
        QStringLiteral("兼容测试模式会扩大端口暴露面，仅建议排查网络问题时临时使用。"),
        content);
    compatHint->setObjectName(QStringLiteral("settingsCompatHint"));
    compatHint->setWordWrap(true);
    compatHint->setStyleSheet(UiStyle::pillStyle(QStringLiteral("#fff4d6"),
                                                 QStringLiteral("#8a5a00")));

    rootLayout->addWidget(sectionLabel(QStringLiteral("网络"), content));
    rootLayout->addWidget(formFrame);
    rootLayout->addWidget(sectionLabel(QStringLiteral("授权网卡"), content));
    rootLayout->addWidget(m_interfaceList);
    rootLayout->addWidget(sectionLabel(QStringLiteral("允许网段 CIDR"), content));
    rootLayout->addWidget(m_allowedCidrsEdit);
    rootLayout->addWidget(sectionLabel(QStringLiteral("手动连接历史"), content));
    rootLayout->addWidget(m_manualPeerList);
    rootLayout->addWidget(compatHint);
    rootLayout->addStretch();

    scroll->setWidget(content);

    auto* tabLayout = new QVBoxLayout(tab);
    tabLayout->setContentsMargins(0, 0, 0, 0);
    tabLayout->addWidget(scroll);

    connect(m_discoveryCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (!m_loading && m_settings) {
            m_settings->setDiscoveryEnabled(checked);
            showRestartHint();
        }
    });
    connect(m_portSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
        if (!m_loading && m_settings) {
            m_settings->setListenPort(static_cast<quint16>(value));
            showRestartHint();
        }
    });
    connect(m_networkModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
                if (!m_loading && m_settings && m_networkModeCombo) {
                    m_settings->setNetworkMode(m_networkModeCombo->currentData().toString());
                    persistInterfaceSelection();
                    showRestartHint();
                }
            });
    connect(m_interfaceList, &QListWidget::itemChanged, this, [this](QListWidgetItem*) {
        if (!m_loading) {
            persistInterfaceSelection();
            showRestartHint();
        }
    });
    connect(m_allowedCidrsEdit, &QPlainTextEdit::textChanged, this, [this]() {
        if (!m_loading && m_settings && m_allowedCidrsEdit) {
            QStringList lines;
            const QStringList parts = m_allowedCidrsEdit->toPlainText().split(
                QRegularExpression(QStringLiteral("[\\n,;]+")), Qt::SkipEmptyParts);
            for (const QString& part : parts) {
                const QString trimmed = part.trimmed();
                if (!trimmed.isEmpty()) {
                    lines.append(trimmed);
                }
            }
            m_settings->setAllowedCidrs(lines.join(QStringLiteral(",")));
            showRestartHint();
        }
    });

    return tab;
}

void SettingsPage::loadSettingsIntoControls()
{
    m_loading = true;

    if (m_displayNameEdit) {
        m_displayNameEdit->setText(m_settings ? m_settings->displayName() : QString());
        m_displayNameEdit->setEnabled(m_settings != nullptr);
    }
    if (m_downloadDirEdit) {
        m_downloadDirEdit->setText(m_settings ? m_settings->downloadDir() : QString());
        m_downloadDirEdit->setEnabled(m_settings != nullptr);
    }
    if (m_autoStartCheck) {
        m_autoStartCheck->setChecked(m_settings ? m_settings->autoStart() : false);
        m_autoStartCheck->setEnabled(m_settings != nullptr);
    }
    if (m_minimizeToTrayCheck) {
        m_minimizeToTrayCheck->setChecked(m_settings ? m_settings->minimizeToTray() : false);
        m_minimizeToTrayCheck->setEnabled(m_settings != nullptr);
    }
    if (m_discoveryCheck) {
        m_discoveryCheck->setChecked(m_settings ? m_settings->discoveryEnabled() : false);
        m_discoveryCheck->setEnabled(m_settings != nullptr);
    }
    if (m_portSpin) {
        m_portSpin->setValue(m_settings ? m_settings->listenPort() : 8787);
        m_portSpin->setEnabled(m_settings != nullptr);
    }
    if (m_networkModeCombo) {
        const QString mode = m_settings ? m_settings->networkMode() : QStringLiteral("secure_lan");
        const int index = m_networkModeCombo->findData(mode);
        m_networkModeCombo->setCurrentIndex(index >= 0 ? index : 0);
        m_networkModeCombo->setEnabled(m_settings != nullptr);
    }
    if (m_allowedCidrsEdit) {
        m_allowedCidrsEdit->setPlainText(splitCsv(m_settings ? m_settings->allowedCidrs() : QString())
                                             .join(QStringLiteral("\n")));
        m_allowedCidrsEdit->setEnabled(m_settings != nullptr);
    }

    rebuildInterfaceList();
    refreshManualPeers();

    m_loading = false;
}

void SettingsPage::rebuildInterfaceList()
{
    if (!m_interfaceList) {
        return;
    }

    m_interfaceList->clear();
    const QStringList selected = splitCsv(m_settings ? m_settings->selectedInterfaces() : QString());
    const QList<NetworkInterfaceInfo> interfaces = InterfaceEnumerator::enumerate();

    if (interfaces.isEmpty()) {
        auto* item = new QListWidgetItem(QStringLiteral("未检测到可显示的 IPv4 网卡"));
        item->setFlags(Qt::ItemIsEnabled);
        m_interfaceList->addItem(item);
        return;
    }

    for (const NetworkInterfaceInfo& iface : interfaces) {
        const QString label = QStringLiteral("%1\n%2  %3")
                                  .arg(iface.displayName,
                                       iface.cidr(),
                                       interfaceTypeText(iface.type));
        auto* item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, iface.id);
        item->setData(Qt::UserRole + 1, iface.cidr());
        item->setSizeHint(QSize(0, 54));
        Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
        if (iface.type != InterfaceType::Physical) {
            flags &= ~Qt::ItemIsEnabled;
        }
        item->setFlags(flags);
        item->setCheckState(selected.contains(iface.id) ? Qt::Checked : Qt::Unchecked);
        m_interfaceList->addItem(item);
    }
}

void SettingsPage::refreshManualPeers()
{
    if (!m_manualPeerList) {
        return;
    }

    m_manualPeerList->clear();
    if (!m_manualPeerRepository) {
        auto* item = new QListWidgetItem(QStringLiteral("暂无手动连接记录"));
        item->setFlags(Qt::ItemIsEnabled);
        m_manualPeerList->addItem(item);
        return;
    }

    const QList<ManualPeer> peers = m_manualPeerRepository->getAllManualPeers();
    if (peers.isEmpty()) {
        auto* item = new QListWidgetItem(QStringLiteral("暂无手动连接记录"));
        item->setFlags(Qt::ItemIsEnabled);
        m_manualPeerList->addItem(item);
        return;
    }

    for (const ManualPeer& peer : peers) {
        const QString timeText = peer.lastSuccessAt.isValid()
            ? peer.lastSuccessAt.toLocalTime().toString(QStringLiteral("MM-dd HH:mm"))
            : QStringLiteral("-");
        auto* item = new QListWidgetItem(QStringLiteral("%1  %2:%3  最近成功 %4")
                                             .arg(peer.name, peer.host)
                                             .arg(peer.port)
                                             .arg(timeText));
        item->setFlags(Qt::ItemIsEnabled);
        item->setSizeHint(QSize(0, 40));
        m_manualPeerList->addItem(item);
    }
}

void SettingsPage::showRestartHint()
{
    if (m_restartHintLabel) {
        m_restartHintLabel->setVisible(true);
    }
}

void SettingsPage::persistInterfaceSelection()
{
    if (!m_interfaceList || !m_settings) {
        return;
    }

    QStringList selectedIds;
    QStringList cidrs;
    const QString mode = m_networkModeCombo
        ? m_networkModeCombo->currentData().toString()
        : QStringLiteral("secure_lan");

    for (int i = 0; i < m_interfaceList->count(); ++i) {
        auto* item = m_interfaceList->item(i);
        if (!item || item->checkState() != Qt::Checked) {
            continue;
        }
        if (mode == QStringLiteral("secure_lan") && !selectedIds.isEmpty()) {
            const QSignalBlocker blocker(m_interfaceList);
            item->setCheckState(Qt::Unchecked);
            continue;
        }
        selectedIds.append(item->data(Qt::UserRole).toString());
        cidrs.append(item->data(Qt::UserRole + 1).toString());
    }

    m_settings->setSelectedInterfaces(selectedIds.join(QStringLiteral(",")));
    m_settings->setAllowedCidrs(cidrs.join(QStringLiteral(",")));

    if (m_allowedCidrsEdit) {
        const QSignalBlocker blocker(m_allowedCidrsEdit);
        m_allowedCidrsEdit->setPlainText(cidrs.join(QStringLiteral("\n")));
    }
}

} // namespace FengSui
