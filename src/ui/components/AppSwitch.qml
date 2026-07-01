// AppSwitch.qml
// 主题化开关：绿色启用态，圆形滑块，平滑过渡。

import QtQuick
import QtQuick.Controls
import FengSui.Ui

Switch {
    id: control

    indicator: Rectangle {
        implicitWidth: 42
        implicitHeight: 24
        radius: Theme.radiusPill
        color: control.checked ? Theme.accent : Theme.surfaceAlt
        border.width: 1
        border.color: control.checked ? Theme.accent : Theme.border
        Behavior on color { ColorAnimation { duration: Theme.animFast } }

        Rectangle {
            width: 18
            height: 18
            radius: Theme.radiusPill
            color: "#ffffff"
            y: 3
            x: control.checked ? parent.width - width - 3 : 3
            Behavior on x { NumberAnimation { duration: Theme.animFast; easing.type: Easing.OutCubic } }
        }
    }

    contentItem: Item {}   // 文案由外部 Label 承载
}
