// tst_share_access_dialog.cpp
// Share access authorization dialog result and remember choice.

#include "ui/share/ShareAccessDialog.h"

#include <QCheckBox>
#include <QPushButton>
#include <QtTest/QtTest>

namespace {

class ShareAccessDialogTest : public QObject {
    Q_OBJECT

private slots:
    void allowAndDenyResultsAreReported();
};

void ShareAccessDialogTest::allowAndDenyResultsAreReported()
{
    FengSui::ShareAccessDialog allowDialog(QStringLiteral("Alice"),
                                           QStringLiteral("Laptop"),
                                           QStringLiteral("Design"));
    auto* remember = allowDialog.findChild<QCheckBox*>(
        QStringLiteral("shareAccessRememberCheck"));
    auto* allowButton = allowDialog.findChild<QPushButton*>(
        QStringLiteral("allowAccessButton"));
    QVERIFY(remember);
    QVERIFY(allowButton);

    remember->setChecked(true);
    allowButton->click();
    QCOMPARE(allowDialog.result(), static_cast<int>(QDialog::Accepted));
    QVERIFY(allowDialog.rememberChoice());

    FengSui::ShareAccessDialog denyDialog(QStringLiteral("Bob"),
                                          QStringLiteral("Desktop"),
                                          QStringLiteral("Docs"));
    auto* denyButton = denyDialog.findChild<QPushButton*>(
        QStringLiteral("denyAccessButton"));
    QVERIFY(denyButton);
    denyButton->click();
    QCOMPARE(denyDialog.result(), static_cast<int>(QDialog::Rejected));
    QVERIFY(!denyDialog.rememberChoice());
}

} // namespace

QTEST_MAIN(ShareAccessDialogTest)

#include "tst_share_access_dialog.moc"
