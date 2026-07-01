// AddPeerDialog.qml
// 手动添加设备对话框：输入 IP + 端口，经 ContactsViewModel.addManualPeer 校验/探测。
// 成功关闭；失败显示错误。探测为阻塞式，探测中禁用输入。

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FengSui.Ui

Dialog {
    id: root

    property int defaultPort: 8787

    title: qsTr("添加设备")
    modal: true
    anchors.centerIn: parent
    width: 400
    padding: Theme.spaceLg
    closePolicy: Popup.CloseOnEscape

    property bool _busy: false

    background: Rectangle {
        radius: Theme.radiusLg
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
    }

    // 打开时重置输入
    onAboutToShow: {
        ipField.text = ""
        portField.text = String(defaultPort)
        statusText.text = ""
        root._busy = false
    }

    contentItem: ColumnLayout {
        spacing: Theme.spaceMd

        Text {
            text: qsTr("添加设备")
            color: Theme.text
            font.pixelSize: Theme.fontLg
            font.weight: Font.Bold
        }
        Text {
            text: qsTr("输入对方的 IP 地址与端口，验证可达后加入列表")
            color: Theme.textMuted
            font.pixelSize: Theme.fontSm
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }

        GridLayout {
            columns: 2
            columnSpacing: Theme.spaceMd
            rowSpacing: Theme.spaceSm
            Layout.fillWidth: true

            Text {
                text: qsTr("IP 地址")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSm
            }
            AppTextField {
                id: ipField
                Layout.fillWidth: true
                enabled: !root._busy
                placeholderText: qsTr("例如 192.168.1.100")
            }

            Text {
                text: qsTr("端口")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSm
            }
            AppTextField {
                id: portField
                Layout.fillWidth: true
                enabled: !root._busy
                placeholderText: "8787"
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator { bottom: 1; top: 65535 }
            }
        }

        Text {
            id: statusText
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            visible: text.length > 0
            color: Theme.danger
            font.pixelSize: Theme.fontSm
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spaceSm
            spacing: Theme.spaceSm

            Item { Layout.fillWidth: true }

            PrimaryButton {
                variant: "secondary"
                text: qsTr("取消")
                enabled: !root._busy
                onClicked: root.reject()
            }
            PrimaryButton {
                variant: "primary"
                text: root._busy ? qsTr("连接中…") : qsTr("连接")
                enabled: !root._busy
                onClicked: {
                    statusText.text = ""
                    root._busy = true
                    // addManualPeer 是阻塞探测，返回空串成功、否则为错误信息
                    var err = AppController.contacts.addManualPeer(
                                ipField.text, parseInt(portField.text || "0"))
                    root._busy = false
                    if (err === "") {
                        root.accept()
                    } else {
                        statusText.text = err
                    }
                }
            }
        }
    }
}
