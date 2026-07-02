// AppComboBox.qml
// 统一风格的下拉选择框：主题化背景/边框/弹出菜单，消除系统原生样式在暗色主题下的突兀感。
// textRole / valueRole 直接使用 ComboBox 原生属性，调用方按需设置即可。

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FengSui.Ui

ComboBox {
    id: control

    hoverEnabled: true
    font.pixelSize: Theme.fontMd
    font.family: Theme.fontFamily
    leftPadding: Theme.spaceMd
    rightPadding: Theme.spaceLg + Theme.spaceMd   // 给 indicator 留空间
    implicitHeight: 38

    delegate: ItemDelegate {
        id: option

        required property int index

        width: control.popup.width - control.popup.leftPadding - control.popup.rightPadding
        height: 36
        hoverEnabled: true
        highlighted: control.highlightedIndex === index

        contentItem: RowLayout {
            spacing: Theme.spaceSm

            Text {
                Layout.fillWidth: true
                text: control.textAt(option.index)
                color: control.currentIndex === option.index ? Theme.text : Theme.textSecondary
                font: control.font
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            AppIcon {
                visible: control.currentIndex === option.index
                name: "check"
                size: 16
                color: Theme.accent
                strokeWidth: 2.4
            }
        }

        background: Rectangle {
            radius: Theme.radiusSm
            color: control.currentIndex === option.index
                   ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, Theme.dark ? 0.20 : 0.12)
                   : (option.highlighted || option.hovered
                      ? Theme.surfaceAlt
                      : "transparent")
            border.width: control.currentIndex === option.index ? 1 : 0
            border.color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, Theme.dark ? 0.42 : 0.26)
        }

        onClicked: {
            control.currentIndex = option.index
            control.activated(option.index)
            control.popup.close()
            control.forceActiveFocus()
        }
    }

    // ---- 下拉弹出面板 ----
    popup: Popup {
        id: comboPopup

        property int maxPopupHeight: 280

        y: control.height + 2
        width: control.width
        implicitHeight: Math.min(optionList.contentHeight, maxPopupHeight) + topPadding + bottomPadding
        padding: Theme.spaceXs
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

        // 主题色注入 palette，默认 ItemDelegate 会自动读取
        palette.text: Theme.text
        palette.base: Theme.surface
        palette.highlight: Theme.accent
        palette.highlightedText: Theme.accentText
        palette.buttonText: Theme.text
        palette.windowText: Theme.text

        background: Rectangle {
            radius: Theme.radiusMd
            color: Theme.surface
            border.width: 1
            border.color: Theme.border
        }

        contentItem: ListView {
            id: optionList

            clip: true
            implicitHeight: Math.min(contentHeight, comboPopup.maxPopupHeight)
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            boundsBehavior: Flickable.StopAtBounds
            spacing: 2
            ScrollIndicator.vertical: ScrollIndicator { }
        }
    }

    // ---- 内容区 ----
    contentItem: Text {
        text: control.displayText
        font: control.font
        color: Theme.text
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    // ---- 背景 ----
    background: Rectangle {
        radius: Theme.radiusMd
        color: control.down || control.popup.visible
               ? Theme.surfaceAlt
               : (control.hovered ? Theme.surface : Theme.bg)
        border.width: 1
        border.color: control.activeFocus || control.popup.visible
                      ? Theme.accent
                      : (control.hovered ? Theme.textFaint : Theme.border)
        Behavior on color { ColorAnimation { duration: Theme.animFast } }
        Behavior on border.color { ColorAnimation { duration: Theme.animFast } }
    }

    // ---- 下拉箭头 ----
    indicator: AppIcon {
        id: arrow
        x: control.width - width - Theme.spaceMd
        y: (control.availableHeight - height) / 2
        name: "chevronDown"
        size: 16
        color: control.popup.visible || control.hovered ? Theme.text : Theme.textSecondary
        rotation: control.popup.visible ? 180 : 0
        Behavior on rotation { NumberAnimation { duration: Theme.animFast; easing.type: Easing.OutCubic } }
    }
}
