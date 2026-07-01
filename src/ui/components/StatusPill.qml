// StatusPill.qml
// 状态徽章：圆角胶囊，可带前置圆点。用于在线/离线、传输状态等标签。

import QtQuick
import FengSui.Ui

Rectangle {
    id: root

    property string text: ""
    property color pillColor: Theme.accent     // 文字与圆点颜色
    property bool showDot: false               // 是否显示前置状态圆点
    property color background: Qt.rgba(pillColor.r, pillColor.g, pillColor.b, 0.14)

    implicitHeight: 22
    implicitWidth: row.implicitWidth + Theme.spaceMd
    radius: Theme.radiusPill
    color: background

    Row {
        id: row
        anchors.centerIn: parent
        spacing: Theme.spaceXs

        Rectangle {
            visible: root.showDot
            width: 7
            height: 7
            radius: Theme.radiusPill
            color: root.pillColor
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: root.text
            color: root.pillColor
            font.pixelSize: Theme.fontXs
            font.weight: Font.DemiBold
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
