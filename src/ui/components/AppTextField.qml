// AppTextField.qml
// 统一风格的输入框：主题化背景/边框，聚焦时强调色描边。

import QtQuick
import QtQuick.Controls
import FengSui.Ui

TextField {
    id: control

    color: Theme.text
    placeholderTextColor: Theme.textFaint
    font.pixelSize: Theme.fontMd
    font.family: Theme.fontFamily
    selectByMouse: true
    leftPadding: Theme.spaceMd
    rightPadding: Theme.spaceMd
    implicitHeight: 38

    background: Rectangle {
        radius: Theme.radiusMd
        color: Theme.bg
        border.width: 1
        border.color: control.activeFocus ? Theme.accent : Theme.border
        Behavior on border.color { ColorAnimation { duration: Theme.animFast } }
    }
}
