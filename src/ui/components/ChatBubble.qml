// ChatBubble.qml
// 聊天气泡：自己发出靠右（强调底色），对方靠左（表面底色）。
// 底部显示时间与发送状态图标（⏳发送中 / ✓已发 / ✓✓已达 / ✗失败）。

import QtQuick
import QtQuick.Layouts
import FengSui.Ui

Item {
    id: root

    property bool outgoing: false
    property string content: ""
    property int status: 0        // Enums.MessageStatus
    property string time: ""
    property int maxBubbleWidth: 420

    implicitHeight: bubble.height + Theme.spaceXs * 2

    Rectangle {
        id: bubble
        width: Math.min(root.maxBubbleWidth, contentCol.implicitWidth + Theme.spaceMd * 2)
        height: contentCol.implicitHeight + Theme.spaceSm * 2
        radius: Theme.radiusMd
        color: root.outgoing ? Theme.bubbleOut : Theme.bubbleIn
        border.width: root.outgoing ? 0 : 1
        border.color: Theme.bubbleInBorder
        anchors.right: root.outgoing ? parent.right : undefined
        anchors.left: root.outgoing ? undefined : parent.left
        y: Theme.spaceXs

        ColumnLayout {
            id: contentCol
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Theme.spaceSm
            spacing: 2

            Text {
                text: root.content
                color: root.outgoing ? Theme.bubbleOutText : Theme.bubbleInText
                font.pixelSize: Theme.fontMd
                wrapMode: Text.Wrap
                Layout.maximumWidth: root.maxBubbleWidth - Theme.spaceMd * 2
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: Theme.spaceXs

                Text {
                    text: root.time
                    color: root.outgoing ? Qt.rgba(1,1,1,0.7) : Theme.textFaint
                    font.pixelSize: Theme.fontXs
                }

                // 状态图标（仅自己发出的消息显示）
                AppIcon {
                    visible: root.outgoing
                    size: 13
                    strokeWidth: 2
                    name: {
                        switch (root.status) {
                        case 0: return "clock"        // Sending
                        case 1: return "check"        // Sent
                        case 2: return "checkDouble"  // Delivered
                        case 3: return "x"            // Failed
                        default: return "clock"
                        }
                    }
                    color: root.status === 3 ? Theme.danger : Qt.rgba(1,1,1,0.85)
                }
            }
        }
    }
}
