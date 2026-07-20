// TransferCenterPage.qml
// 传输中心：统一查看文件收发进度、状态与失败原因。支持关键词搜索 + 状态筛选。
// 数据来自 AppController.transfer（TransferViewModel + 过滤代理模型）。

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FengSui.Ui
import "../qml/Format.mjs" as Fmt

Item {
    id: root

    Rectangle { anchors.fill: parent; color: Theme.bg }

    // 状态 → 配色
    function statusColor(st) {
        switch (st) {
        case 3: return Theme.online    // Completed
        case 1: // Rejected
        case 4: // Failed
        case 5: return Theme.danger    // Cancelled
        case 2: return Theme.accent    // Transferring
        default: return Theme.warning  // Waiting
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceLg
        spacing: Theme.spaceMd

        PageHeader {
            Layout.fillWidth: true
            title: qsTr("传输中心")
            subtitle: qsTr("统一查看文件收发、进度和失败原因")
        }

        // 工具栏：搜索 + 状态筛选
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceSm

            AppTextField {
                id: searchField
                Layout.preferredWidth: 260
                placeholderText: qsTr("搜索文件或设备")
                onTextChanged: AppController.transfer.tasks.keyword = text
            }

            AppComboBox {
                id: statusCombo
                Layout.preferredWidth: 140
                font.pixelSize: Theme.fontSm
                textRole: "text"
                valueRole: "value"
                model: [
                    { text: qsTr("全部"),   value: -1 },
                    { text: qsTr("等待中"), value: 0 },
                    { text: qsTr("传输中"), value: 2 },
                    { text: qsTr("已完成"), value: 3 },
                    { text: qsTr("失败"),   value: 4 },
                    { text: qsTr("已拒绝"), value: 1 },
                    { text: qsTr("已取消"), value: 5 }
                ]
                currentIndex: {
                    var f = AppController.transfer.tasks.statusFilter
                    switch (f) {
                    case 0: return 1
                    case 2: return 2
                    case 3: return 3
                    case 4: return 4
                    case 1: return 5
                    case 5: return 6
                    default: return 0
                    }
                }
                onActivated: AppController.transfer.tasks.statusFilter = currentValue
            }

            Item { Layout.fillWidth: true }
        }

        // 任务列表
        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: Theme.spaceSm
            model: AppController.transfer.tasks

            delegate: Rectangle {
                id: rowItem
                required property int index
                required property string transferId
                required property int direction
                required property string fileName
                required property string peerName
                required property int status
                required property double progress
                required property var transferredBytes
                required property var fileSize
                required property string errorMessage

                width: ListView.view.width
                implicitHeight: 76
                radius: Theme.radiusMd
                color: hover.hovered ? Theme.surfaceAlt : Theme.surface
                border.width: 1
                border.color: Theme.border
                Behavior on color { ColorAnimation { duration: Theme.animFast } }

                HoverHandler { id: hover }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spaceMd
                    anchors.rightMargin: Theme.spaceMd
                    spacing: Theme.spaceMd

                    // 方向图标
                    AppIcon {
                        name: rowItem.direction === 0 ? "arrowUp" : "arrowDown"
                        size: 20
                        color: rowItem.direction === 0 ? Theme.info : Theme.accent
                        Layout.alignment: Qt.AlignVCenter
                    }

                    // 文件 + 对端 + 进度
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.spaceSm
                            Text {
                                text: rowItem.fileName
                                color: Theme.text
                                font.pixelSize: Theme.fontMd
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            Text {
                                text: Fmt.direction(rowItem.direction) + " · " + rowItem.peerName
                                color: Theme.textMuted
                                font.pixelSize: Theme.fontSm
                            }
                        }

                        // 传输中显示进度条，否则显示大小/错误
                        ThinProgressBar {
                            Layout.fillWidth: true
                            visible: rowItem.status === 2   // Transferring
                            value: rowItem.progress
                            barColor: root.statusColor(rowItem.status)
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: rowItem.status !== 2
                            text: {
                                if (rowItem.status === 4 && rowItem.errorMessage.length > 0)
                                    return rowItem.errorMessage
                                return Fmt.bytes(rowItem.transferredBytes) + " / " + Fmt.bytes(rowItem.fileSize)
                            }
                            color: rowItem.status === 4 ? Theme.danger : Theme.textMuted
                            font.pixelSize: Theme.fontSm
                            elide: Text.ElideRight
                        }
                    }

                    // 状态胶囊
                    StatusPill {
                        text: Fmt.statusText(rowItem.status)
                        pillColor: root.statusColor(rowItem.status)
                        Layout.alignment: Qt.AlignVCenter
                    }

                    // 取消按钮（仅进行中/等待中）
                    PrimaryButton {
                        variant: "ghost"
                        visible: rowItem.status === 0 || rowItem.status === 2
                        implicitHeight: 30
                        leftPadding: Theme.spaceSm
                        rightPadding: Theme.spaceSm
                        text: qsTr("取消")
                        onClicked: AppController.transfer.cancelTransfer(rowItem.transferId)
                    }
                }

                // 渐入
                NumberAnimation on opacity {
                    running: true
                    from: 0; to: 1
                    duration: Theme.animMed
                    easing.type: Easing.OutCubic
                }
            }

            EmptyState {
                anchors.fill: parent
                visible: list.count === 0
                iconName: "transfer"
                title: qsTr("暂无传输任务")
                subtitle: qsTr("从聊天窗口拖入文件，或接收对方发来的文件后将在此显示")
            }
        }
    }
}
