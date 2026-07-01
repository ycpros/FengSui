// ContactsPage.qml
// 联系人页：展示 BeaconService 发现的在线设备，支持手动添加。
// 数据来自 AppController.contacts（ContactsViewModel + PeerListModel）。

import QtQuick
import QtQuick.Layouts
import FengSui.Ui

Item {
    id: root

    Rectangle { anchors.fill: parent; color: Theme.bg }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceLg
        spacing: Theme.spaceMd

        PageHeader {
            Layout.fillWidth: true
            title: qsTr("联系人")
            subtitle: qsTr("局域网内发现的在线设备")

            PrimaryButton {
                variant: "primary"
                text: qsTr("添加设备")
                onClicked: addDialog.open()
            }
        }

        // 设备列表
        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: Theme.spaceSm
            model: AppController.contacts.peers

            // delegate 根为 DeviceCard，其属性名与 model role 同名，QML 自动绑定；
            // 仅 index 需显式 required 以传给 activatePeer。
            delegate: DeviceCard {
                required property int index
                width: ListView.view.width
                onActivated: AppController.contacts.activatePeer(index)

                // 列表项渐入
                NumberAnimation on opacity {
                    running: true
                    from: 0; to: 1
                    duration: Theme.animMed
                    easing.type: Easing.OutCubic
                }
            }

            // 空态
            EmptyState {
                anchors.fill: parent
                visible: list.count === 0
                iconName: "contacts"
                title: qsTr("暂无在线设备")
                subtitle: qsTr("确保设备在同一局域网，或点击「添加设备」手动连接")
            }
        }
    }

    AddPeerDialog { id: addDialog }
}
