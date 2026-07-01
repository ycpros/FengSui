// NavItem.qml
// 左侧导航栏的单个条目：图标字符 + 文字，选中态高亮，悬停淡入，点击回弹。

import QtQuick
import FengSui.Ui

Item {
    id: root

    property string label: ""
    property string iconName: ""    // AppIcon 图标名
    property bool selected: false
    property int badgeCount: 0      // 未读等徽标数，0 不显示

    signal clicked()

    implicitHeight: 40

    Rectangle {
        id: bg
        anchors.fill: parent
        anchors.leftMargin: Theme.spaceSm
        anchors.rightMargin: Theme.spaceSm
        radius: Theme.radiusMd
        color: root.selected ? Theme.surfaceAlt
                             : (hover.hovered ? Theme.surface : "transparent")

        Behavior on color { ColorAnimation { duration: Theme.animFast } }

        // 选中态左侧强调条
        Rectangle {
            width: 3
            height: parent.height * 0.5
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 2
            radius: Theme.radiusPill
            color: Theme.accent
            visible: root.selected
        }

        Row {
            anchors.left: parent.left
            anchors.leftMargin: Theme.spaceMd
            anchors.right: parent.right
            anchors.rightMargin: Theme.spaceMd
            anchors.verticalCenter: parent.verticalCenter
            spacing: Theme.spaceMd

            AppIcon {
                name: root.iconName
                size: 19
                color: root.selected ? Theme.accent : Theme.textMuted
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: root.label
                font.pixelSize: Theme.fontMd
                font.family: Theme.fontFamily
                font.weight: root.selected ? Font.DemiBold : Font.Medium
                color: root.selected ? Theme.text : Theme.textSecondary
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        // 未读徽标（靠右）
        Rectangle {
            visible: root.badgeCount > 0
            anchors.right: parent.right
            anchors.rightMargin: Theme.spaceMd
            anchors.verticalCenter: parent.verticalCenter
            height: 18
            width: Math.max(18, badgeText.implicitWidth + 10)
            radius: Theme.radiusPill
            color: Theme.accent
            Text {
                id: badgeText
                anchors.centerIn: parent
                text: root.badgeCount > 99 ? "99+" : root.badgeCount
                color: Theme.accentText
                font.pixelSize: Theme.fontXs
                font.weight: Font.DemiBold
            }
        }
    }

    HoverHandler { id: hover }

    TapHandler {
        id: tap
        onTapped: root.clicked()
    }

    // 点击回弹：按下缩小，松开回弹
    scale: tap.pressed ? 0.97 : 1.0
    Behavior on scale { NumberAnimation { duration: Theme.animFast; easing.type: Easing.OutBack } }
}
