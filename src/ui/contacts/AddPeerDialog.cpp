// AddPeerDialog.cpp
// 手动添加设备对话框实现。

#include "ui/contacts/AddPeerDialog.h"
#include "core/TcpProbe.h"

#include <QDialogButtonBox>
#include <QDateTime>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkInterface>
#include <QPushButton>
#include <QVBoxLayout>

namespace FengSui {

namespace {

constexpr int kDefaultWidth = 380;
constexpr int kDefaultHeight = 220;
constexpr int kProbeTimeoutMs = 3000;

} // namespace

AddPeerDialog::AddPeerDialog(quint16 defaultPort, QWidget* parent)
    : QDialog(parent)
    , m_defaultPort(defaultPort)
{
    setupUi();
}

PeerInfo AddPeerDialog::resultPeer() const
{
    return m_resultPeer;
}

void AddPeerDialog::setupUi()
{
    setWindowTitle(QString::fromUtf8("添加设备"));
    setFixedSize(kDefaultWidth, kDefaultHeight);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(12);

    // IP 地址行
    auto* ipRow = new QHBoxLayout();
    auto* ipLabel = new QLabel(QString::fromUtf8("IP 地址"), this);
    ipLabel->setFixedWidth(60);
    m_ipEdit = new QLineEdit(this);
    m_ipEdit->setPlaceholderText(QString::fromUtf8("例如 192.168.1.100"));
    ipRow->addWidget(ipLabel);
    ipRow->addWidget(m_ipEdit, 1);

    // 端口行
    auto* portRow = new QHBoxLayout();
    auto* portLabel = new QLabel(QString::fromUtf8("端口"), this);
    portLabel->setFixedWidth(60);
    m_portEdit = new QLineEdit(this);
    m_portEdit->setPlaceholderText(QString::fromUtf8("8787"));
    m_portEdit->setText(QString::number(m_defaultPort));
    portRow->addWidget(portLabel);
    portRow->addWidget(m_portEdit, 1);

    // 状态消息标签
    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignLeft);
    m_statusLabel->setStyleSheet("padding-left: 4px;");
    m_statusLabel->setVisible(false);

    // 按钮
    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    m_connectBtn = new QPushButton(QString::fromUtf8("连接"), this);
    m_connectBtn->setDefault(true);
    m_connectBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #4a90d9; color: white; border: none;"
        "  border-radius: 4px; padding: 6px 24px; font-size: 13px;"
        "}"
        "QPushButton:hover { background-color: #357abd; }"
        "QPushButton:disabled { background-color: #aaa; }"
    );
    m_cancelBtn = new QPushButton(QString::fromUtf8("取消"), this);
    m_cancelBtn->setStyleSheet(
        "QPushButton {"
        "  border: 1px solid #ccc; border-radius: 4px;"
        "  padding: 6px 24px; font-size: 13px;"
        "}"
        "QPushButton:hover { background-color: #eee; }"
    );
    btnRow->addWidget(m_connectBtn);
    btnRow->addWidget(m_cancelBtn);

    layout->addLayout(ipRow);
    layout->addLayout(portRow);
    layout->addWidget(m_statusLabel);
    layout->addStretch();
    layout->addLayout(btnRow);

    connect(m_connectBtn, &QPushButton::clicked, this, &AddPeerDialog::onConnectClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

bool AddPeerDialog::validateInput(QString& errorOut) const
{
    const QString ip = m_ipEdit->text().trimmed();
    if (ip.isEmpty()) {
        errorOut = QString::fromUtf8("请输入 IP 地址");
        return false;
    }

    QHostAddress addr(ip);
    if (addr.isNull() || addr.protocol() != QAbstractSocket::IPv4Protocol) {
        errorOut = QString::fromUtf8("IP 地址格式无效");
        return false;
    }

    const QString portText = m_portEdit->text().trimmed();
    if (portText.isEmpty()) {
        errorOut = QString::fromUtf8("请输入端口号");
        return false;
    }

    bool portOk = false;
    const int port = portText.toInt(&portOk);
    if (!portOk || port < 1 || port > 65535) {
        errorOut = QString::fromUtf8("端口号范围 1-65535");
        return false;
    }

    return true;
}

bool AddPeerDialog::isSelfConnection(const QString& ip, quint16 port) const
{
    const QHostAddress targetAddr(ip);

    // 直接是回环地址
    if (targetAddr.isLoopback()) {
        return true;
    }

    // 遍历本机所有非回环 IPv4 地址
    const QList<QHostAddress> localAddrs = QNetworkInterface::allAddresses();
    for (const QHostAddress& localAddr : localAddrs) {
        if (localAddr.protocol() != QAbstractSocket::IPv4Protocol) {
            continue;
        }
        if (localAddr.isLoopback()) {
            continue;
        }
        if (localAddr == targetAddr) {
            return true;
        }
    }

    return false;
}

void AddPeerDialog::setInputsEnabled(bool enabled)
{
    m_ipEdit->setEnabled(enabled);
    m_portEdit->setEnabled(enabled);
    m_connectBtn->setEnabled(enabled);
    m_cancelBtn->setEnabled(enabled);
}

void AddPeerDialog::onConnectClicked()
{
    m_statusLabel->setVisible(true);
    m_statusLabel->setStyleSheet("padding-left: 4px; color: #333;");

    QString error;
    if (!validateInput(error)) {
        m_statusLabel->setText(error);
        m_statusLabel->setStyleSheet("padding-left: 4px; color: #cc0000;");
        return;
    }

    const QString ip = m_ipEdit->text().trimmed();
    bool portOk = false;
    const quint16 port = static_cast<quint16>(m_portEdit->text().trimmed().toUShort(&portOk));

    // 自连接检测
    if (isSelfConnection(ip, port)) {
        m_statusLabel->setText(QString::fromUtf8("不能连接本机地址"));
        m_statusLabel->setStyleSheet("padding-left: 4px; color: #cc0000;");
        return;
    }

    // 禁用输入，开始探测
    setInputsEnabled(false);
    m_statusLabel->setText(QString::fromUtf8("正在连接..."));
    m_statusLabel->setStyleSheet("padding-left: 4px; color: #333;");

    QString probeError;
    if (TcpProbe::probe(ip, port, kProbeTimeoutMs, probeError)) {
        // 探测成功，构造 PeerInfo
        m_resultPeer.peerId = QString::fromUtf8("manual:%1:%2").arg(ip).arg(port);
        m_resultPeer.displayName = QString::fromUtf8("%1:%2").arg(ip).arg(port);
        m_resultPeer.deviceName = QString::fromUtf8("手动添加");
        m_resultPeer.ip = ip;
        m_resultPeer.port = port;
        m_resultPeer.online = true;
        m_resultPeer.lastSeenAt = QDateTime::currentDateTimeUtc();

        accept();
    } else {
        m_statusLabel->setText(probeError);
        m_statusLabel->setStyleSheet("padding-left: 4px; color: #cc0000;");
        setInputsEnabled(true);
    }
}

} // namespace FengSui
