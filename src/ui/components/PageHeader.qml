// PageHeader.qml
// 页面标题区：主标题 + 副标题，右侧可放操作按钮（default 属性 actions）。

import QtQuick
import QtQuick.Layouts
import FengSui.Ui

Item {
    id: root

    property string title: ""
    property string subtitle: ""
    default property alias actions: actionRow.data

    implicitHeight: Math.max(titleCol.implicitHeight, actionRow.implicitHeight)

    ColumnLayout {
        id: titleCol
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2

        Text {
            text: root.title
            color: Theme.text
            font.pixelSize: Theme.fontXl
            font.weight: Font.Bold
        }
        Text {
            text: root.subtitle
            visible: text.length > 0
            color: Theme.textMuted
            font.pixelSize: Theme.fontSm
        }
    }

    RowLayout {
        id: actionRow
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.spaceSm
    }
}
