// DeviceCard.qml
// 设备卡片：OS 徽标 + 昵称/设备名/端点 + 在线状态胶囊。悬停高亮，双击激活。

import QtQuick
import QtQuick.Layouts
import FengSui.Ui

Rectangle {
    id: root

    property string displayName: ""
    property string deviceName: ""
    property string endpoint: ""
    property string os: ""
    property bool online: true
    property bool shareEnabled: false

    signal activated()

    implicitHeight: 68
    radius: Theme.radiusMd
    color: hover.hovered ? Theme.surfaceAlt : Theme.surface
    border.width: 1
    border.color: Theme.border

    Behavior on color { ColorAnimation { duration: Theme.animFast } }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spaceMd
        anchors.rightMargin: Theme.spaceMd
        spacing: Theme.spaceMd

        OsBadge { os: root.os }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            RowLayout {
                spacing: Theme.spaceSm
                Text {
                    text: root.displayName
                    color: Theme.text
                    font.pixelSize: Theme.fontMd
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }
                AppIcon {
                    visible: root.shareEnabled
                    name: "folder"
                    size: 14
                    color: Theme.textMuted
                    Layout.alignment: Qt.AlignVCenter
                }
            }
            Text {
                text: root.deviceName + "  ·  " + root.endpoint
                color: Theme.textMuted
                font.pixelSize: Theme.fontSm
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        StatusPill {
            text: root.online ? qsTr("在线") : qsTr("离线")
            pillColor: root.online ? Theme.online : Theme.offline
            showDot: true
        }
    }

    HoverHandler { id: hover }
    TapHandler {
        id: tap
        acceptedButtons: Qt.LeftButton
        onDoubleTapped: root.activated()
    }

    scale: tap.pressed ? 0.99 : 1.0
    Behavior on scale { NumberAnimation { duration: Theme.animFast } }
}
