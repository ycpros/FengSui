// ChatPage.qml
// 聊天页：左会话列表 + 右聊天区（消息气泡 + 输入框 + 拖拽发送）。
// 数据来自 AppController.chat（ChatViewModel + 会话/消息模型）。

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FengSui.Ui
import "../qml/Format.js" as Fmt

Item {
    id: root

    readonly property var vm: AppController.chat

    Rectangle { anchors.fill: parent; color: Theme.bg }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ---- 左：会话列表 ----
        Rectangle {
            Layout.preferredWidth: 280
            Layout.fillHeight: true
            color: Theme.surface

            Rectangle {
                anchors.right: parent.right
                width: 1; height: parent.height; color: Theme.border
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spaceMd
                spacing: Theme.spaceSm

                Text {
                    text: qsTr("消息")
                    color: Theme.text
                    font.pixelSize: Theme.fontLg
                    font.weight: Font.Bold
                }

                ListView {
                    id: convList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 2
                    model: root.vm.conversations

                    delegate: Rectangle {
                        required property int index
                        required property string conversationId
                        required property string displayName
                        required property string lastMessage
                        required property int unreadCount
                        required property bool online

                        width: ListView.view.width
                        implicitHeight: 60
                        radius: Theme.radiusMd
                        color: convHover.hovered ? Theme.surfaceAlt : "transparent"
                        Behavior on color { ColorAnimation { duration: Theme.animFast } }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.spaceSm
                            anchors.rightMargin: Theme.spaceSm
                            spacing: Theme.spaceSm

                            // 头像圆点（含在线态）
                            Rectangle {
                                Layout.preferredWidth: 40
                                Layout.preferredHeight: 40
                                radius: Theme.radiusPill
                                color: Theme.surfaceAlt
                                Text {
                                    anchors.centerIn: parent
                                    text: displayName.length > 0 ? displayName.charAt(0).toUpperCase() : "?"
                                    color: Theme.textSecondary
                                    font.pixelSize: Theme.fontMd
                                    font.weight: Font.Bold
                                }
                                Rectangle {
                                    visible: online
                                    width: 11; height: 11; radius: Theme.radiusPill
                                    color: Theme.online
                                    border.width: 2; border.color: Theme.surface
                                    anchors.right: parent.right
                                    anchors.bottom: parent.bottom
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text {
                                    text: displayName
                                    color: Theme.text
                                    font.pixelSize: Theme.fontMd
                                    font.weight: Font.DemiBold
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: lastMessage
                                    color: Theme.textMuted
                                    font.pixelSize: Theme.fontSm
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }

                            // 未读徽标
                            Rectangle {
                                visible: unreadCount > 0
                                Layout.preferredHeight: 18
                                Layout.preferredWidth: Math.max(18, unreadText.implicitWidth + 10)
                                radius: Theme.radiusPill
                                color: Theme.accent
                                Text {
                                    id: unreadText
                                    anchors.centerIn: parent
                                    text: unreadCount > 99 ? "99+" : unreadCount
                                    color: Theme.accentText
                                    font.pixelSize: Theme.fontXs
                                    font.weight: Font.DemiBold
                                }
                            }
                        }

                        HoverHandler { id: convHover }
                        TapHandler { onTapped: root.vm.openConversationRow(index) }
                    }

                    EmptyState {
                        anchors.fill: parent
                        visible: convList.count === 0
                        iconName: "chat"
                        title: qsTr("暂无会话")
                        subtitle: qsTr("在联系人页双击设备开始聊天")
                    }
                }
            }
        }

        // ---- 右：聊天区 ----
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // 未选择会话时的空态
            EmptyState {
                anchors.fill: parent
                visible: !root.vm.hasActiveConversation
                iconName: "chat"
                title: qsTr("选择一个会话开始聊天")
            }

            ColumnLayout {
                anchors.fill: parent
                visible: root.vm.hasActiveConversation
                spacing: 0

                // 头部
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 56
                    color: Theme.surface
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spaceLg
                        anchors.rightMargin: Theme.spaceLg
                        spacing: Theme.spaceSm
                        Text {
                            text: root.vm.activePeerName
                            color: Theme.text
                            font.pixelSize: Theme.fontLg
                            font.weight: Font.DemiBold
                        }
                        StatusPill {
                            text: root.vm.activePeerOnline ? qsTr("在线") : qsTr("离线")
                            pillColor: root.vm.activePeerOnline ? Theme.online : Theme.offline
                            showDot: true
                        }
                        Item { Layout.fillWidth: true }
                    }
                }

                // 离线提示
                Rectangle {
                    Layout.fillWidth: true
                    visible: !root.vm.activePeerOnline
                    color: Qt.rgba(Theme.warning.r, Theme.warning.g, Theme.warning.b, 0.14)
                    implicitHeight: 30
                    Text {
                        anchors.centerIn: parent
                        text: qsTr("对方当前离线，消息将在其上线后尝试送达")
                        color: Theme.warning
                        font.pixelSize: Theme.fontXs
                    }
                }

                // 消息列表
                ListView {
                    id: msgList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 2
                    leftMargin: Theme.spaceLg
                    rightMargin: Theme.spaceLg
                    topMargin: Theme.spaceMd
                    bottomMargin: Theme.spaceMd
                    model: root.vm.messages
                    // 新消息自动滚到底
                    onCountChanged: positionViewAtEnd()

                    delegate: Item {
                        required property bool isOutgoing
                        required property string content
                        required property int status
                        required property var createdAt
                        width: ListView.view.width - Theme.spaceLg * 2

                        implicitHeight: bubble.implicitHeight
                        ChatBubble {
                            id: bubble
                            width: parent.width
                            outgoing: isOutgoing
                            content: content
                            status: status
                            time: Qt.formatDateTime(createdAt, "hh:mm")
                        }
                    }
                }

                // 拖拽提示 + 输入区
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 68
                    color: Theme.surface
                    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.border }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: Theme.spaceSm
                        spacing: Theme.spaceSm

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: Theme.radiusMd
                            color: Theme.bg
                            border.width: 1
                            border.color: dropArea.containsDrag ? Theme.accent : Theme.border

                            ScrollView {
                                anchors.fill: parent
                                anchors.margins: Theme.spaceXs
                                TextArea {
                                    id: input
                                    placeholderText: dropArea.containsDrag
                                                     ? qsTr("松开以发送文件")
                                                     : qsTr("输入消息，Enter 发送，Shift+Enter 换行；可拖入文件")
                                    placeholderTextColor: Theme.textFaint
                                    color: Theme.text
                                    font.pixelSize: Theme.fontMd
                                    wrapMode: TextArea.Wrap
                                    background: null
                                    Keys.onReturnPressed: (event) => {
                                        if (event.modifiers & Qt.ShiftModifier) {
                                            event.accepted = false   // 换行
                                        } else {
                                            root.vm.sendText(input.text)
                                            input.clear()
                                            event.accepted = true
                                        }
                                    }
                                }
                            }

                            DropArea {
                                id: dropArea
                                anchors.fill: parent
                                onDropped: (drop) => {
                                    if (drop.hasUrls) {
                                        root.vm.sendFiles(drop.urls)
                                        drop.accept()
                                    }
                                }
                            }
                        }

                        PrimaryButton {
                            variant: "primary"
                            text: qsTr("发送")
                            Layout.alignment: Qt.AlignBottom
                            enabled: input.text.trim().length > 0
                            onClicked: {
                                root.vm.sendText(input.text)
                                input.clear()
                            }
                        }
                    }
                }
            }
        }
    }
}
