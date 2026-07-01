// PrimaryButton.qml
// 主/次按钮：默认强调色填充（primary），variant 可选 "secondary"（描边）/ "ghost"（透明）。
// 悬停变色，按下回弹，禁用置灰。

import QtQuick
import QtQuick.Controls
import FengSui.Ui

Button {
    id: control

    // "primary" | "secondary" | "ghost"
    property string variant: "primary"

    font.pixelSize: Theme.fontMd
    font.family: Theme.fontFamily
    font.weight: Font.DemiBold
    implicitHeight: 36
    leftPadding: Theme.spaceMd
    rightPadding: Theme.spaceMd

    scale: control.down ? 0.97 : 1.0
    Behavior on scale { NumberAnimation { duration: Theme.animFast; easing.type: Easing.OutBack } }

    contentItem: Text {
        text: control.text
        font: control.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: {
            if (!control.enabled)
                return Theme.textFaint
            if (control.variant === "primary")
                return Theme.accentText
            return Theme.accent
        }
    }

    background: Rectangle {
        radius: Theme.radiusMd
        border.width: control.variant === "secondary" ? 1 : 0
        border.color: Theme.border
        color: {
            if (control.variant === "primary") {
                if (!control.enabled)
                    return Theme.surfaceAlt
                return control.hovered ? Theme.accentHover : Theme.accent
            }
            // secondary / ghost
            if (control.hovered && control.enabled)
                return Theme.surfaceAlt
            return "transparent"
        }
        Behavior on color { ColorAnimation { duration: Theme.animFast } }
    }
}
