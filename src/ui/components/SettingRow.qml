// SettingRow.qml
// 设置项行：左侧标题 + 说明，右侧控件槽（default 属性 control）。

import QtQuick
import QtQuick.Layouts
import FengSui.Ui

RowLayout {
    id: root

    property string label: ""
    property string description: ""
    default property alias control: controlSlot.data

    spacing: Theme.spaceMd
    Layout.fillWidth: true

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2
        Text {
            text: root.label
            color: Theme.text
            font.pixelSize: Theme.fontMd
        }
        Text {
            text: root.description
            visible: text.length > 0
            color: Theme.textMuted
            font.pixelSize: Theme.fontXs
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }

    RowLayout {
        id: controlSlot
        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
        spacing: Theme.spaceSm
    }
}
