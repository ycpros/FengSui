// IncomingTransferDialog.qml
// 接收文件确认弹窗：对方发来文件传输请求时弹出，接受/拒绝。
// 由 Main.qml 监听 AppController.transfer.incomingRequest 触发。

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FengSui.Ui
import "../qml/Format.js" as Fmt

Dialog {
    id: root

    property string transferId: ""
    property string peerName: ""
    property string fileName: ""
    property var fileSize: 0

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
            AppIcon { name: "download"; size: 22; color: Theme.accent }
            Text {
                text: qsTr("接收文件")
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
            text: qsTr("%1 想向你发送文件").arg(root.peerName)
        }

        // 文件信息卡片
        Rectangle {
            Layout.fillWidth: true
            radius: Theme.radiusMd
            color: Theme.bg
            border.width: 1
            border.color: Theme.border
            implicitHeight: 56
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spaceMd
                anchors.rightMargin: Theme.spaceMd
                spacing: Theme.spaceSm
                AppIcon { name: "file"; size: 22; color: Theme.textMuted }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: root.fileName
                        color: Theme.text
                        font.pixelSize: Theme.fontMd
                        font.weight: Font.DemiBold
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }
                    Text {
                        text: Fmt.bytes(root.fileSize)
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontXs
                    }
                }
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
                    AppController.transfer.rejectTransfer(root.transferId)
                    root.close()
                }
            }
            PrimaryButton {
                variant: "primary"
                text: qsTr("接收")
                onClicked: {
                    AppController.transfer.acceptTransfer(root.transferId)
                    root.close()
                }
            }
        }
    }
}
