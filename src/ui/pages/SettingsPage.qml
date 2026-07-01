// SettingsPage.qml
// 设置页：常规 / 网络 / 网络诊断 三个分区（顶部 Tab 切换）。
// 所有设置即时写入 AppController.settings（SettingsViewModel），网络项变更提示需重启。

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import FengSui.Ui

Item {
    id: root

    readonly property var vm: AppController.settings
    property int tab: 0

    Rectangle { anchors.fill: parent; color: Theme.bg }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceLg
        spacing: Theme.spaceMd

        PageHeader {
            Layout.fillWidth: true
            title: qsTr("设置")
            subtitle: qsTr("常规、网络与诊断")
        }

        // Tab 切换条
        RowLayout {
            spacing: Theme.spaceXs
            Repeater {
                model: [qsTr("常规"), qsTr("网络"), qsTr("网络诊断")]
                delegate: Rectangle {
                    required property int index
                    required property string modelData
                    implicitHeight: 34
                    implicitWidth: tabLabel.implicitWidth + Theme.spaceLg
                    radius: Theme.radiusMd
                    color: root.tab === index ? Theme.surfaceAlt : "transparent"
                    Behavior on color { ColorAnimation { duration: Theme.animFast } }
                    Text {
                        id: tabLabel
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

        // 网络变更需重启提示
        Rectangle {
            Layout.fillWidth: true
            visible: root.vm.needsRestart && root.tab !== 0
            radius: Theme.radiusMd
            color: Qt.rgba(Theme.warning.r, Theme.warning.g, Theme.warning.b, 0.14)
            implicitHeight: 36
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spaceMd
                spacing: Theme.spaceSm
                AppIcon { name: "alert"; size: 16; color: Theme.warning }
                Text {
                    text: qsTr("网络设置已更改，重启应用后生效")
                    color: Theme.warning
                    font.pixelSize: Theme.fontSm
                }
            }
        }

        // 内容区（按 tab 切换）
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.tab

            // ---- 常规 ----
            ScrollView {
                clip: true
                contentWidth: availableWidth
                ColumnLayout {
                    width: parent.width
                    spacing: Theme.spaceLg

                    SectionCard {
                        Layout.fillWidth: true
                        title: qsTr("个人")
                        SettingRow {
                            label: qsTr("昵称")
                            description: qsTr("局域网内其他设备看到的名称")
                            AppTextField {
                                Layout.preferredWidth: 220
                                text: root.vm.displayName
                                onEditingFinished: root.vm.displayName = text
                            }
                        }
                        SettingRow {
                            label: qsTr("下载目录")
                            description: root.vm.downloadDir
                            PrimaryButton {
                                variant: "secondary"
                                text: qsTr("更改")
                                onClicked: folderDialog.open()
                            }
                        }
                    }

                    SectionCard {
                        Layout.fillWidth: true
                        title: qsTr("外观")
                        SettingRow {
                            label: qsTr("主题")
                            description: qsTr("跟随系统，或强制亮色/暗色")
                            ComboBox {
                                Layout.preferredWidth: 140
                                model: [
                                    { text: qsTr("跟随系统"), value: "system" },
                                    { text: qsTr("亮色"),     value: "light" },
                                    { text: qsTr("暗色"),     value: "dark" }
                                ]
                                textRole: "text"
                                valueRole: "value"
                                currentIndex: {
                                    var m = ThemeController.mode
                                    return m === "light" ? 1 : (m === "dark" ? 2 : 0)
                                }
                                onActivated: ThemeController.mode = currentValue
                            }
                        }
                    }

                    SectionCard {
                        Layout.fillWidth: true
                        title: qsTr("系统")
                        SettingRow {
                            label: qsTr("开机自启")
                            AppSwitch {
                                checked: root.vm.autoStart
                                onToggled: root.vm.autoStart = checked
                            }
                        }
                        SettingRow {
                            label: qsTr("关闭时最小化到托盘")
                            AppSwitch {
                                checked: root.vm.minimizeToTray
                                onToggled: root.vm.minimizeToTray = checked
                            }
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            // ---- 网络 ----
            ScrollView {
                clip: true
                contentWidth: availableWidth
                ColumnLayout {
                    width: parent.width
                    spacing: Theme.spaceLg

                    SectionCard {
                        Layout.fillWidth: true
                        title: qsTr("发现与端口")
                        SettingRow {
                            label: qsTr("局域网自动发现")
                            AppSwitch {
                                checked: root.vm.discoveryEnabled
                                onToggled: root.vm.discoveryEnabled = checked
                            }
                        }
                        SettingRow {
                            label: qsTr("监听端口")
                            AppTextField {
                                Layout.preferredWidth: 120
                                text: String(root.vm.listenPort)
                                inputMethodHints: Qt.ImhDigitsOnly
                                validator: IntValidator { bottom: 1; top: 65535 }
                                onEditingFinished: root.vm.listenPort = parseInt(text || "0")
                            }
                        }
                        SettingRow {
                            label: qsTr("网络模式")
                            ComboBox {
                                Layout.preferredWidth: 160
                                model: [
                                    { text: qsTr("安全内网"),   value: "secure_lan" },
                                    { text: qsTr("多网卡内网"), value: "multi_lan" },
                                    { text: qsTr("兼容测试"),   value: "compat_test" }
                                ]
                                textRole: "text"
                                valueRole: "value"
                                currentIndex: {
                                    var m = root.vm.networkMode
                                    return m === "multi_lan" ? 1 : (m === "compat_test" ? 2 : 0)
                                }
                                onActivated: root.vm.networkMode = currentValue
                            }
                        }
                    }

                    SectionCard {
                        Layout.fillWidth: true
                        title: qsTr("授权网卡")
                        Repeater {
                            model: root.vm.interfaces
                            delegate: RowLayout {
                                required property var modelData
                                Layout.fillWidth: true
                                spacing: Theme.spaceMd
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    Text {
                                        text: modelData.name + "  " + modelData.cidr
                                        color: Theme.text
                                        font.pixelSize: Theme.fontSm
                                    }
                                    Text {
                                        text: modelData.type + (modelData.physical ? qsTr(" · 物理网卡") : "")
                                        color: Theme.textMuted
                                        font.pixelSize: Theme.fontXs
                                    }
                                }
                                AppSwitch {
                                    checked: modelData.selected
                                    enabled: modelData.physical
                                    onToggled: root.vm.toggleInterface(modelData.id, checked)
                                }
                            }
                        }
                    }

                    SectionCard {
                        Layout.fillWidth: true
                        title: qsTr("允许网段 (CIDR)")
                        AppTextField {
                            Layout.fillWidth: true
                            text: root.vm.allowedCidrs
                            placeholderText: "192.168.1.0/24, 10.0.0.0/24"
                            onEditingFinished: root.vm.allowedCidrs = text
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            // ---- 网络诊断 ----
            ScrollView {
                clip: true
                contentWidth: availableWidth
                ColumnLayout {
                    width: parent.width
                    spacing: Theme.spaceLg

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: qsTr("运行状态摘要")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSm
                            font.weight: Font.DemiBold
                        }
                        Item { Layout.fillWidth: true }
                        PrimaryButton {
                            variant: "secondary"
                            text: qsTr("重新检测")
                            onClicked: root.vm.refresh()
                        }
                    }

                    // 摘要胶囊
                    Flow {
                        Layout.fillWidth: true
                        spacing: Theme.spaceSm
                        StatusPill { text: qsTr("模式：") + root.vm.diagMode; pillColor: Theme.info }
                        StatusPill { text: qsTr("端口：") + root.vm.diagPort; pillColor: Theme.accent }
                        StatusPill { text: qsTr("在线设备：") + root.vm.diagOnlineCount; pillColor: Theme.online }
                    }

                    SectionCard {
                        Layout.fillWidth: true
                        title: qsTr("允许网段")
                        Text {
                            text: root.vm.diagAllowedCidrs
                            color: Theme.text
                            font.pixelSize: Theme.fontSm
                            font.family: Theme.monoFamily
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }

                    SectionCard {
                        Layout.fillWidth: true
                        title: qsTr("本机网卡")
                        Repeater {
                            model: root.vm.interfaces
                            delegate: RowLayout {
                                required property var modelData
                                Layout.fillWidth: true
                                spacing: Theme.spaceMd
                                Text {
                                    text: modelData.name
                                    color: Theme.text
                                    font.pixelSize: Theme.fontSm
                                    Layout.preferredWidth: 120
                                }
                                Text {
                                    text: modelData.cidr
                                    color: Theme.textMuted
                                    font.pixelSize: Theme.fontSm
                                    font.family: Theme.monoFamily
                                    Layout.fillWidth: true
                                }
                                StatusPill {
                                    text: modelData.type
                                    pillColor: modelData.physical ? Theme.online : Theme.textMuted
                                }
                            }
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }
        }
    }

    FolderDialog {
        id: folderDialog
        title: qsTr("选择下载目录")
        onAccepted: {
            // selectedFolder 是 file:// URL，转成本地路径
            root.vm.downloadDir = selectedFolder.toString().replace(/^file:\/\/\//, "")
        }
    }
}
