// BrandMark.qml
// 抽象化的烽火信标：同心光环与中心火点表达本地发现、联结与传递，
// 不使用具象国风装饰，可在深浅主题与不同尺寸下复用。

import QtQuick
import FengSui.Ui

Item {
    id: root

    property int size: 40
    property bool filled: true
    property color markColor: Theme.accent

    implicitWidth: size
    implicitHeight: size

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusMd
        color: root.filled ? root.markColor : Theme.brandSoft
        border.width: root.filled ? 0 : 1
        border.color: Qt.rgba(root.markColor.r, root.markColor.g, root.markColor.b, 0.30)
    }

    Item {
        anchors.centerIn: parent
        width: parent.width * 0.64
        height: width

        readonly property color ink: root.filled ? Theme.accentText : root.markColor

        Rectangle {
            anchors.centerIn: parent
            width: parent.width
            height: width
            radius: width / 2
            color: "transparent"
            border.width: 1
            border.color: Qt.rgba(parent.ink.r, parent.ink.g, parent.ink.b, 0.45)
        }
        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.58
            height: width
            radius: width / 2
            color: "transparent"
            border.width: 1
            border.color: parent.ink
        }
        Rectangle {
            anchors.centerIn: parent
            width: Math.max(5, parent.width * 0.20)
            height: width
            radius: width / 2
            color: parent.ink
        }
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.top
            anchors.bottomMargin: -parent.height * 0.10
            width: Math.max(2, parent.width * 0.08)
            height: parent.height * 0.18
            radius: width / 2
            color: parent.ink
        }
    }
}
