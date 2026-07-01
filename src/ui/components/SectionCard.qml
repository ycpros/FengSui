// SectionCard.qml
// 分组卡片：可选标题 + 内容列（default 属性）。用于设置/诊断分区。

import QtQuick
import QtQuick.Layouts
import FengSui.Ui

ColumnLayout {
    id: root

    property string title: ""
    default property alias content: inner.data

    spacing: Theme.spaceSm

    Text {
        visible: root.title.length > 0
        text: root.title
        color: Theme.textSecondary
        font.pixelSize: Theme.fontSm
        font.weight: Font.DemiBold
        Layout.leftMargin: Theme.spaceXs
    }

    Rectangle {
        Layout.fillWidth: true
        radius: Theme.radiusLg
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
        implicitHeight: inner.implicitHeight + Theme.spaceMd * 2

        ColumnLayout {
            id: inner
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Theme.spaceMd
            spacing: Theme.spaceMd
        }
    }
}
