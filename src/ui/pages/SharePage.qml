// SharePage.qml
// 共享文件页：两个 Tab —— 浏览共享（远程源→目录→文件下载）/ 我的共享（增删启停）。
// 数据来自 AppController.share（ShareViewModel）。

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import FengSui.Ui
import "../qml/Format.js" as Fmt

Item {
    id: root

    readonly property var vm: AppController.share
    property int tab: 0

    Rectangle { anchors.fill: parent; color: Theme.bg }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceLg
        spacing: Theme.spaceMd

        PageHeader {
            Layout.fillWidth: true
            title: qsTr("共享文件")
            subtitle: qsTr("浏览其他设备的共享，或管理本机共享目录")

            PrimaryButton {
                variant: "primary"
                visible: root.tab === 1
                text: qsTr("添加共享")
                onClicked: folderDialog.open()
            }
        }

        // Tab 切换
        RowLayout {
            spacing: Theme.spaceXs
            Repeater {
                model: [qsTr("浏览共享"), qsTr("我的共享")]
                delegate: Rectangle {
                    required property int index
                    required property string modelData
                    implicitHeight: 34
                    implicitWidth: tabText.implicitWidth + Theme.spaceLg
                    radius: Theme.radiusMd
                    color: root.tab === index ? Theme.surfaceAlt : "transparent"
                    Behavior on color { ColorAnimation { duration: Theme.animFast } }
                    Text {
                        id: tabText
                        anchors.centerIn: parent
                        text: modelData
                        color: root.tab === index ? Theme.text : Theme.textMuted
                        font.pixelSize: Theme.fontMd
                        font.weight: root.tab === index ? Font.DemiBold : Font.Normal
                    }
                    TapHandler { onTapped: root.tab = index }
                }
            }
            Item { Layout.fillWidth: true }
        }

        // 状态提示
        Text {
            Layout.fillWidth: true
            visible: root.vm.statusMessage.length > 0
            text: root.vm.statusMessage
            color: Theme.textMuted
            font.pixelSize: Theme.fontSm
            elide: Text.ElideRight
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.tab

            // ---- 浏览共享 ----
            RowLayout {
                spacing: Theme.spaceMd

                // 左：源列表 + 该源的共享目录
                Rectangle {
                    Layout.preferredWidth: 280
                    Layout.fillHeight: true
                    radius: Theme.radiusLg
                    color: Theme.surface
                    border.width: 1
                    border.color: Theme.border

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Theme.spaceMd
                        spacing: Theme.spaceSm

                        Text {
                            text: qsTr("共享源")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSm
                            font.weight: Font.DemiBold
                        }
                        // 在线共享源列表
                        ListView {
                            Layout.fillWidth: true
                            Layout.preferredHeight: Math.min(contentHeight, 160)
                            clip: true
                            spacing: 2
                            model: root.vm.sources
                            delegate: Rectangle {
                                required property var modelData
                                width: ListView.view.width
                                implicitHeight: 40
                                radius: Theme.radiusSm
                                color: srcHover.hovered ? Theme.surfaceAlt : "transparent"
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: Theme.spaceSm
                                    anchors.rightMargin: Theme.spaceSm
                                    AppIcon { name: "contacts"; size: 16; color: Theme.textMuted }
                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.displayName
                                        color: Theme.text
                                        font.pixelSize: Theme.fontSm
                                        elide: Text.ElideRight
                                    }
                                }
                                HoverHandler { id: srcHover }
                                TapHandler { onTapped: root.vm.selectSource(modelData.peerId) }
                            }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

                        Text {
                            text: qsTr("共享目录")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSm
                            font.weight: Font.DemiBold
                        }
                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            spacing: 2
                            model: root.vm.remoteShares
                            delegate: Rectangle {
                                required property var modelData
                                width: ListView.view.width
                                implicitHeight: 40
                                radius: Theme.radiusSm
                                color: shHover.hovered ? Theme.surfaceAlt : "transparent"
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: Theme.spaceSm
                                    anchors.rightMargin: Theme.spaceSm
                                    AppIcon { name: "folder"; size: 16; color: Theme.accent }
                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.displayName
                                        color: Theme.text
                                        font.pixelSize: Theme.fontSm
                                        elide: Text.ElideRight
                                    }
                                }
                                HoverHandler { id: shHover }
                                TapHandler { onTapped: root.vm.openShare(modelData.shareId) }
                            }
                        }
                    }
                }

                // 右：文件浏览
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: Theme.radiusLg
                    color: Theme.surface
                    border.width: 1
                    border.color: Theme.border

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Theme.spaceMd
                        spacing: Theme.spaceSm

                        // 面包屑
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("位置：") + (root.vm.currentPath.length > 0 ? root.vm.currentPath : "/")
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontSm
                            font.family: Theme.monoFamily
                            elide: Text.ElideMiddle
                        }
                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

                        ListView {
                            id: fileList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            spacing: 2
                            model: root.vm.remoteItems
                            delegate: Rectangle {
                                required property var modelData
                                width: ListView.view.width
                                implicitHeight: 44
                                radius: Theme.radiusSm
                                color: itemHover.hovered ? Theme.surfaceAlt : "transparent"
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: Theme.spaceSm
                                    anchors.rightMargin: Theme.spaceSm
                                    spacing: Theme.spaceSm
                                    AppIcon {
                                        name: modelData.isDir ? "folder" : "file"
                                        size: 18
                                        color: modelData.isDir ? Theme.accent : Theme.textMuted
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.name
                                        color: Theme.text
                                        font.pixelSize: Theme.fontSm
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        visible: !modelData.isDir
                                        text: Fmt.bytes(modelData.size)
                                        color: Theme.textMuted
                                        font.pixelSize: Theme.fontXs
                                    }
                                    AppIcon {
                                        visible: !modelData.isDir
                                        name: "download"
                                        size: 16
                                        color: dlHover.hovered ? Theme.accent : Theme.textMuted
                                        HoverHandler { id: dlHover }
                                        TapHandler {
                                            onTapped: root.vm.downloadItem(
                                                          root.vm.currentPath === "" ? "" : modelData.path,
                                                          modelData.path)
                                        }
                                    }
                                    AppIcon {
                                        visible: modelData.isDir
                                        name: "chevronRight"
                                        size: 16
                                        color: Theme.textFaint
                                    }
                                }
                                HoverHandler { id: itemHover }
                                TapHandler {
                                    onTapped: {
                                        if (modelData.isDir)
                                            root.vm.openPath(root.vm.currentShareId || "", modelData.path)
                                    }
                                }
                            }

                            EmptyState {
                                anchors.fill: parent
                                visible: fileList.count === 0
                                iconName: "folder"
                                title: qsTr("选择左侧共享目录浏览")
                            }
                        }
                    }
                }
            }

            // ---- 我的共享 ----
            ColumnLayout {
                spacing: Theme.spaceSm

                ListView {
                    id: myList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: Theme.spaceSm
                    model: root.vm.myShares
                    delegate: Rectangle {
                        required property var modelData
                        width: ListView.view.width
                        implicitHeight: 64
                        radius: Theme.radiusMd
                        color: Theme.surface
                        border.width: 1
                        border.color: Theme.border

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.spaceMd
                            anchors.rightMargin: Theme.spaceMd
                            spacing: Theme.spaceMd

                            AppIcon { name: "folder"; size: 22; color: Theme.accent }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text {
                                    text: modelData.displayName
                                    color: Theme.text
                                    font.pixelSize: Theme.fontMd
                                    font.weight: Font.DemiBold
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: modelData.localPath
                                    color: Theme.textMuted
                                    font.pixelSize: Theme.fontXs
                                    font.family: Theme.monoFamily
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }
                            }

                            AppSwitch {
                                checked: modelData.isActive
                                onToggled: root.vm.setSharedFolderActive(modelData.shareId, checked)
                            }

                            AppIcon {
                                name: "trash"
                                size: 18
                                color: rmHover.hovered ? Theme.danger : Theme.textMuted
                                HoverHandler { id: rmHover }
                                TapHandler { onTapped: root.vm.removeSharedFolder(modelData.shareId) }
                            }
                        }
                    }

                    EmptyState {
                        anchors.fill: parent
                        visible: myList.count === 0
                        iconName: "folder"
                        title: qsTr("尚未共享任何目录")
                        subtitle: qsTr("点击「添加共享」选择要对外共享的本地目录")
                    }
                }
            }
        }
    }

    FolderDialog {
        id: folderDialog
        title: qsTr("选择共享目录")
        onAccepted: root.vm.addSharedFolder(selectedFolder.toString().replace(/^file:\/\/\//, ""))
    }
}
