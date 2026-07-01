// OnboardingWizard.qml
// 首次启动向导：昵称 → 网络发现 → 下载目录 →（多网卡时）授权网卡。
// 铺满窗口，完成时调用 OnboardingViewModel.finish()。

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import FengSui.Ui

Item {
    id: root

    readonly property var vm: AppController.onboarding
    property int step: 0
    // 步骤总数：基础 3 步，多网卡再 +1
    readonly property int stepCount: vm.needsInterfaceStep ? 4 : 3
    readonly property bool isLast: step === stepCount - 1

    // 遮罩背景（铺满，拦截下层交互）
    Rectangle { anchors.fill: parent; color: Theme.bg }

    // 居中卡片
    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width - Theme.spaceXl * 2, 520)
        height: Math.min(parent.height - Theme.spaceXl * 2, 480)
        radius: Theme.radiusLg
        color: Theme.surface
        border.width: 1
        border.color: Theme.border

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.spaceLg
            spacing: Theme.spaceLg

            // 品牌头
            RowLayout {
                spacing: Theme.spaceSm
                Rectangle {
                    Layout.preferredWidth: 36; Layout.preferredHeight: 36
                    radius: Theme.radiusMd; color: Theme.accent
                    Text { anchors.centerIn: parent; text: "烽"; color: Theme.accentText
                           font.pixelSize: Theme.fontLg; font.weight: Font.Bold }
                }
                Text {
                    text: qsTr("欢迎使用烽燧")
                    color: Theme.text
                    font.pixelSize: Theme.fontXl
                    font.weight: Font.Bold
                }
            }

            // 步骤内容
            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: root.step

                // 步骤 0：昵称
                ColumnLayout {
                    spacing: Theme.spaceMd
                    Text { text: qsTr("设置昵称"); color: Theme.text
                           font.pixelSize: Theme.fontLg; font.weight: Font.DemiBold }
                    Text { text: qsTr("局域网内其他设备将看到这个名称")
                           color: Theme.textMuted; font.pixelSize: Theme.fontSm }
                    AppTextField {
                        Layout.fillWidth: true
                        text: root.vm.nickname
                        placeholderText: qsTr("例如 我的电脑")
                        onTextChanged: root.vm.nickname = text
                    }
                    Item { Layout.fillHeight: true }
                }

                // 步骤 1：网络发现
                ColumnLayout {
                    spacing: Theme.spaceMd
                    Text { text: qsTr("网络发现"); color: Theme.text
                           font.pixelSize: Theme.fontLg; font.weight: Font.DemiBold }
                    Text { text: qsTr("开启后可自动发现同一局域网内的其他设备")
                           color: Theme.textMuted; font.pixelSize: Theme.fontSm
                           Layout.fillWidth: true; wrapMode: Text.WordWrap }
                    RowLayout {
                        spacing: Theme.spaceSm
                        AppSwitch {
                            checked: root.vm.discoveryEnabled
                            onToggled: root.vm.discoveryEnabled = checked
                        }
                        Text { text: qsTr("启用局域网自动发现"); color: Theme.text
                               font.pixelSize: Theme.fontMd }
                    }
                    Item { Layout.fillHeight: true }
                }

                // 步骤 2：下载目录
                ColumnLayout {
                    spacing: Theme.spaceMd
                    Text { text: qsTr("默认下载目录"); color: Theme.text
                           font.pixelSize: Theme.fontLg; font.weight: Font.DemiBold }
                    Text { text: qsTr("接收到的文件将保存到此目录")
                           color: Theme.textMuted; font.pixelSize: Theme.fontSm }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spaceSm
                        AppTextField {
                            Layout.fillWidth: true
                            text: root.vm.downloadDir
                            readOnly: true
                        }
                        PrimaryButton {
                            variant: "secondary"
                            text: qsTr("浏览")
                            onClicked: folderDialog.open()
                        }
                    }
                    Item { Layout.fillHeight: true }
                }

                // 步骤 3：授权网卡（多网卡时）
                ColumnLayout {
                    spacing: Theme.spaceMd
                    Text { text: qsTr("选择授权网卡"); color: Theme.text
                           font.pixelSize: Theme.fontLg; font.weight: Font.DemiBold }
                    Text { text: qsTr("仅在选中的网卡上进行设备发现与通信")
                           color: Theme.textMuted; font.pixelSize: Theme.fontSm
                           Layout.fillWidth: true; wrapMode: Text.WordWrap }
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: Theme.spaceXs
                        model: root.vm.interfaces
                        delegate: RowLayout {
                            required property var modelData
                            width: ListView.view.width
                            spacing: Theme.spaceSm
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text { text: modelData.name; color: Theme.text
                                       font.pixelSize: Theme.fontSm }
                                Text { text: modelData.cidr; color: Theme.textMuted
                                       font.pixelSize: Theme.fontXs; font.family: Theme.monoFamily }
                            }
                            AppSwitch {
                                checked: modelData.selected
                                enabled: modelData.physical
                                onToggled: root.vm.toggleInterface(modelData.id, checked)
                            }
                        }
                    }
                }
            }

            // 步骤指示 + 导航按钮
            RowLayout {
                Layout.fillWidth: true

                // 步骤圆点
                RowLayout {
                    spacing: Theme.spaceXs
                    Repeater {
                        model: root.stepCount
                        delegate: Rectangle {
                            required property int index
                            width: index === root.step ? 20 : 7
                            height: 7
                            radius: Theme.radiusPill
                            color: index === root.step ? Theme.accent : Theme.surfaceAlt
                            Behavior on width { NumberAnimation { duration: Theme.animFast } }
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                PrimaryButton {
                    variant: "ghost"
                    text: qsTr("上一步")
                    visible: root.step > 0
                    onClicked: root.step--
                }
                PrimaryButton {
                    variant: "primary"
                    text: root.isLast ? qsTr("开始使用") : qsTr("下一步")
                    // 昵称步骤要求非空
                    enabled: root.step !== 0 || root.vm.nickname.trim().length > 0
                    onClicked: {
                        if (root.isLast) {
                            root.vm.finish()
                        } else {
                            root.step++
                        }
                    }
                }
            }
        }
    }

    FolderDialog {
        id: folderDialog
        title: qsTr("选择下载目录")
        onAccepted: root.vm.downloadDir = selectedFolder.toString().replace(/^file:\/\/\//, "")
    }
}
