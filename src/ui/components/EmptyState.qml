// EmptyState.qml
// 空态占位：居中大图标 + 标题 + 副标题，用于列表无数据时。

import QtQuick
import FengSui.Ui

Item {
    id: root

    property string iconName: "folder"
    property string title: ""
    property string subtitle: ""

    Column {
        anchors.centerIn: parent
        width: Math.min(parent.width - Theme.spaceXl * 2, 360)
        spacing: Theme.spaceMd

        AppIcon {
            name: root.iconName
            size: 44
            strokeWidth: 1.5
            color: Theme.textFaint
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            text: root.title
            visible: text.length > 0
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.pixelSize: Theme.fontLg
            font.weight: Font.DemiBold
            color: Theme.textMuted
        }

        Text {
            text: root.subtitle
            visible: text.length > 0
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.pixelSize: Theme.fontSm
            color: Theme.textFaint
        }
    }
}
