// AddPeerDialog.h
// 手动添加设备对话框。
// 用户输入 IP:端口后通过 TCP 探针验证可达性，验证通过后将 PeerInfo 返回给调用方。

#pragma once

#include <QDialog>

#include "models/PeerInfo.h"

class QLabel;
class QLineEdit;
class QPushButton;

namespace FengSui {

// 手动添加设备对话框。
// 提供 IP 和端口输入，点击"连接"后通过 TcpProbe 验证 TCP 可达性。
// 调用方通过 resultPeer() 获取验证成功的设备信息。
class AddPeerDialog : public QDialog {
    Q_OBJECT

public:
    // 创建添加设备对话框。
    // defaultPort: 端口输入框默认值（监听端口）。
    // parent: Qt 父对象。
    explicit AddPeerDialog(quint16 defaultPort, QWidget* parent = nullptr);

    // 获取连接验证成功的设备信息。
    // 仅在 exec() 返回 Accepted 后有效。
    PeerInfo resultPeer() const;

private slots:
    // 点击"连接"按钮，执行输入验证 → 自连接检测 → TCP 探针。
    void onConnectClicked();

private:
    // 创建对话框布局和控件。
    void setupUi();

    // 验证 IP 和端口格式合法性。
    // errorOut: 失败时写入错误描述。
    // 返回 true 表示格式有效。
    bool validateInput(QString& errorOut) const;

    // 检测目标 IP:端口是否为本地地址。
    // 防止用户手动连接自己而浪费资源。
    bool isSelfConnection(const QString& ip, quint16 port) const;

    // 设置输入控件和连接按钮的启用状态。
    void setInputsEnabled(bool enabled);

    QLineEdit*      m_ipEdit = nullptr;
    QLineEdit*      m_portEdit = nullptr;
    QLabel*         m_statusLabel = nullptr;
    QPushButton*    m_connectBtn = nullptr;
    QPushButton*    m_cancelBtn = nullptr;
    PeerInfo        m_resultPeer;
    quint16         m_defaultPort = 8787;
};

} // namespace FengSui
