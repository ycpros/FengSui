// ShareAccessDialog.qml
// 共享访问授权弹窗：对方请求访问本机共享时弹出，允许/拒绝 + 记住选择。
// 由 Main.qml 监听 AppController.share.accessRequested 触发。

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FengSui.Ui

Dialog {
    id: root

    property string requesterName: ""
    property string deviceName: ""
    property string shareName: ""

    modal: true
    anchors.centerIn: parent
    width: 420
    padding: Theme.spaceLg
    closePolicy: Popup.NoAutoClose

    background: Rectangle {
        radius: Theme.radiusLg
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
    }

    contentItem: ColumnLayout {
        spacing: Theme.spaceMd

        RowLayout {
            spacing: Theme.spaceSm
            AppIcon { name: "folder"; size: 22; color: Theme.accent }
            Text {
                text: qsTr("共享访问请求")
                color: Theme.text
                font.pixelSize: Theme.fontLg
                font.weight: Font.Bold
            }
        }

        Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSm
            text: qsTr("%1（%2）请求访问共享目录「%3」")
                    .arg(root.requesterName)
                    .arg(root.deviceName)
                    .arg(root.shareName)
        }

        RowLayout {
            spacing: Theme.spaceSm
            CheckBox { id: rememberBox }
            Text {
                text: qsTr("记住我的选择，此设备后续访问不再询问")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSm
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spaceSm
            spacing: Theme.spaceSm
            Item { Layout.fillWidth: true }
            PrimaryButton {
                variant: "secondary"
                text: qsTr("拒绝")
                onClicked: {
                    AppController.share.resolveAccess(false, rememberBox.checked)
                    root.close()
                }
            }
            PrimaryButton {
                variant: "primary"
                text: qsTr("允许")
                onClicked: {
                    AppController.share.resolveAccess(true, rememberBox.checked)
                    root.close()
                }
            }
        }
    }

    onAboutToShow: rememberBox.checked = false
}
