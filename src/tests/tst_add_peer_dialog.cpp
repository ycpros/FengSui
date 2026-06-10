// tst_add_peer_dialog.cpp
// 手动添加设备对话框策略校验测试。

#include "models/NetworkPolicy.h"
#include "ui/contacts/AddPeerDialog.h"

#include <QLabel>
#include <QLineEdit>
#include <QMetaObject>
#include <QtTest/QtTest>

namespace {

class AddPeerDialogTest : public QObject {
    Q_OBJECT

private slots:
    void cidrOutsideTargetIsRejectedBeforeProbe();
};

void AddPeerDialogTest::cidrOutsideTargetIsRejectedBeforeProbe()
{
    FengSui::NetworkPolicy policy;
    policy.setAllowedCidrs({QStringLiteral("192.168.10.0/24")});

    FengSui::AddPeerDialog dialog(8787, &policy);
    auto* ipEdit = dialog.findChild<QLineEdit*>(QStringLiteral("ipEdit"));
    auto* portEdit = dialog.findChild<QLineEdit*>(QStringLiteral("portEdit"));
    auto* statusLabel = dialog.findChild<QLabel*>(QStringLiteral("statusLabel"));
    QVERIFY(ipEdit);
    QVERIFY(portEdit);
    QVERIFY(statusLabel);

    ipEdit->setText(QStringLiteral("10.0.0.2"));
    portEdit->setText(QStringLiteral("8787"));

    QVERIFY(QMetaObject::invokeMethod(&dialog, "onConnectClicked"));
    QCOMPARE(statusLabel->text(), QStringLiteral("目标 IP 不在允许网段内"));
}

} // namespace

QTEST_MAIN(AddPeerDialogTest)

#include "tst_add_peer_dialog.moc"
