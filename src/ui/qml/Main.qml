// Main.qml
// 应用主壳：左侧固定导航栏（260px）+ 右侧页面栈（StackLayout 保留各页状态）。
// 阶段 2：5 个占位页 + 导航切换 + 暗色切换验证；阶段 3 起替换为真实页面。

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as Platform
import FengSui.Ui

ApplicationWindow {
    id: root
    width: 1100
    height: 720
    minimumWidth: 900
    minimumHeight: 600
    visible: true
    title: qsTr("烽燧 FengSui")
    color: Theme.bg

    // 关闭窗口时：仅在托盘后端可用且设置了「最小化到托盘」时隐藏；
    // 若托盘不可用则正常退出，避免后台存活却没有入口恢复窗口。
    onClosing: (close) => {
        if (AppController.settings.minimizeToTray && tray.available) {
            close.accepted = false
            root.hide()
        }
    }

    // 系统托盘。Windows 上 Qt.labs.platform 通过 QtWidgets/QSystemTrayIcon 后端实现，
    // UI 本身仍保持 QML/Qt Quick。
    Platform.SystemTrayIcon {
        id: tray
        visible: available
        icon.source: "qrc:/qt/qml/FengSui.Ui/assets/tray.svg"
        tooltip: qsTr("烽燧 FengSui")

        onActivated: (reason) => {
            if (reason === Platform.SystemTrayIcon.Trigger
                || reason === Platform.SystemTrayIcon.DoubleClick) {
                root.show()
                root.raise()
                root.requestActivate()
            }
        }

        menu: Platform.Menu {
            Platform.MenuItem {
                text: qsTr("打开烽燧")
                onTriggered: { root.show(); root.raise(); root.requestActivate() }
            }
            Platform.MenuSeparator {}
            Platform.MenuItem {
                text: qsTr("退出")
                onTriggered: Qt.quit()
            }
        }
    }

    // 导航项定义（单色线性图标，AppIcon 按名渲染）
    readonly property var navItems: [
        { label: qsTr("消息"),     icon: "chat" },
        { label: qsTr("联系人"),   icon: "contacts" },
        { label: qsTr("传输中心"), icon: "transfer" },
        { label: qsTr("共享文件"), icon: "folder" },
        { label: qsTr("设置"),     icon: "settings" }
    ]

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ---- 左侧导航栏 ----
        Rectangle {
            Layout.preferredWidth: Theme.navWidth
            Layout.fillHeight: true
            color: Theme.surface

            // 右边框分隔
            Rectangle {
                anchors.right: parent.right
                width: 1
                height: parent.height
                color: Theme.border
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spaceMd
                spacing: Theme.spaceMd

                // 品牌头部
                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: Theme.spaceSm
                    spacing: Theme.spaceSm

                    Rectangle {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        radius: Theme.radiusMd
                        color: Theme.accent
                        Text {
                            anchors.centerIn: parent
                            text: "烽"
                            color: Theme.accentText
                            font.pixelSize: Theme.fontLg
                            font.weight: Font.Bold
                        }
                    }

                    ColumnLayout {
                        spacing: 0
                        Text {
                            text: qsTr("烽燧")
                            color: Theme.text
                            font.pixelSize: Theme.fontLg
                            font.weight: Font.Bold
                        }
                        Text {
                            text: qsTr("FengSui 局域网传输")
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontXs
                        }
                    }

                    Item { Layout.fillWidth: true }
                }

                // 在线状态
                StatusPill {
                    text: qsTr("在线")
                    pillColor: Theme.online
                    showDot: true
                    Layout.leftMargin: Theme.spaceSm
                }

                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

                // 导航列表
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceXs

                    Repeater {
                        model: root.navItems
                        delegate: NavItem {
                            required property int index
                            required property var modelData
                            Layout.fillWidth: true
                            label: modelData.label
                            iconName: modelData.icon
                            selected: AppController.currentIndex === index
                            onClicked: AppController.currentIndex = index
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                // 底部：昵称 + 暗色切换
                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: Theme.spaceSm
                    Layout.rightMargin: Theme.spaceSm
                    Layout.bottomMargin: Theme.spaceSm
                    spacing: Theme.spaceSm

                    Text {
                        Layout.fillWidth: true
                        text: AppController.displayName
                        color: Theme.text
                        font.pixelSize: Theme.fontSm
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }

                    // 暗色/亮色快速切换（正式入口在设置页，这里便于验证）
                    Rectangle {
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 32
                        radius: Theme.radiusMd
                        color: themeToggleHover.hovered ? Theme.surfaceAlt : "transparent"
                        Behavior on color { ColorAnimation { duration: Theme.animFast } }

                        AppIcon {
                            anchors.centerIn: parent
                            name: Theme.dark ? "moon" : "sun"
                            size: 18
                            color: Theme.textSecondary
                        }
                        HoverHandler { id: themeToggleHover }
                        TapHandler {
                            onTapped: ThemeController.mode = Theme.dark ? "light" : "dark"
                        }
                    }
                }
            }
        }

        // ---- 右侧页面栈 ----
        // 索引与 navItems 一致：0 消息 / 1 联系人 / 2 传输中心 / 3 共享文件 / 4 设置。
        // 阶段 3 逐个把占位替换为真实页面。
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: AppController.currentIndex

            ChatPage {}                                                            // 0
            ContactsPage {}                                                        // 1
            TransferCenterPage {}                                                  // 2
            SharePage {}                                                           // 3
            SettingsPage {}                                                        // 4
        }
    }

    // 共享访问授权弹窗：监听 ShareViewModel 的 accessRequested 信号
    ShareAccessDialog { id: accessDialog }
    Connections {
        target: AppController.share
        function onAccessRequested(requesterName, deviceName, shareName) {
            accessDialog.requesterName = requesterName
            accessDialog.deviceName = deviceName
            accessDialog.shareName = shareName
            accessDialog.open()
        }
    }

    // 接收文件确认弹窗：监听 TransferViewModel 的 incomingRequest 信号
    IncomingTransferDialog { id: incomingDialog }
    Connections {
        target: AppController.transfer
        function onIncomingRequest(transferId, peerName, fileName, fileSize) {
            incomingDialog.transferId = transferId
            incomingDialog.peerName = peerName
            incomingDialog.fileName = fileName
            incomingDialog.fileSize = fileSize
            incomingDialog.open()
        }
    }

    // 首次启动向导：需要时铺满窗口盖在主界面之上；完成后自动隐藏。
    Loader {
        anchors.fill: parent
        active: AppController.onboardingNeeded
        sourceComponent: OnboardingWizard {}
    }
}
