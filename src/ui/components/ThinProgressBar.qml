// ThinProgressBar.qml
// 细进度条：0~1 值，平滑过渡。可指定进度颜色（按状态着色）。

import QtQuick
import FengSui.Ui

Rectangle {
    id: root

    property real value: 0            // 0.0 ~ 1.0
    property color barColor: Theme.accent

    implicitHeight: 6
    radius: Theme.radiusPill
    color: Theme.surfaceAlt

    Rectangle {
        height: parent.height
        radius: parent.radius
        color: root.barColor
        width: parent.width * Math.max(0, Math.min(1, root.value))
        Behavior on width { NumberAnimation { duration: Theme.animFast } }
    }
}
