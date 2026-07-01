// OsBadge.qml
// 操作系统徽标：根据 os 字符串显示简短平台名与配色的小方块。

import QtQuick
import FengSui.Ui

Rectangle {
    id: root

    property string os: ""

    // 平台 → 显示文本与主色
    readonly property var _map: ({
        "windows": { label: "Win",   color: "#3b82f6" },
        "linux":   { label: "Linux", color: "#f59e0b" },
        "macos":   { label: "macOS", color: "#a78bfa" }
    })
    readonly property var _info: _map[String(os).toLowerCase()]
                                 || ({ label: "OS", color: Theme.textMuted })

    implicitWidth: 46
    implicitHeight: 24
    radius: Theme.radiusSm
    color: Qt.rgba(_info.color.r, _info.color.g, _info.color.b, 0.16)

    Text {
        anchors.centerIn: parent
        text: root._info.label
        color: root._info.color
        font.pixelSize: Theme.fontXs
        font.weight: Font.DemiBold
    }
}
