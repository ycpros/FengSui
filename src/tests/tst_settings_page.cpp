// tst_settings_page.cpp
// SettingsPage widget bindings for existing AppSettings values.

#include "app/AppSettings.h"
#include "storage/Database.h"
#include "storage/SettingsRepository.h"
#include "ui/settings/SettingsPage.h"

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QtTest/QtTest>

namespace {

class SettingsPageTest : public QObject {
    Q_OBJECT

private slots:
    void editsArePersistedToSettings();
};

void SettingsPageTest::editsArePersistedToSettings()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    FengSui::Database database(tempDir.filePath(QStringLiteral("settings.db")));
    QVERIFY(database.initialize());
    FengSui::SettingsRepository settingsRepo(&database);
    FengSui::AppSettings settings(&settingsRepo);
    QVERIFY(settings.load());
    settings.setDisplayName(QStringLiteral("old name"));
    settings.setDownloadDir(QStringLiteral("C:/Downloads"));
    settings.setDiscoveryEnabled(true);
    settings.setListenPort(8787);

    FengSui::SettingsPage page(&settings);

    auto* nameEdit = page.findChild<QLineEdit*>(QStringLiteral("settingsDisplayNameEdit"));
    auto* downloadEdit = page.findChild<QLineEdit*>(QStringLiteral("settingsDownloadDirEdit"));
    auto* discoveryCheck = page.findChild<QCheckBox*>(QStringLiteral("settingsDiscoveryCheck"));
    auto* portSpin = page.findChild<QSpinBox*>(QStringLiteral("settingsListenPortSpin"));
    auto* restartHint = page.findChild<QLabel*>(QStringLiteral("settingsRestartHint"));

    QVERIFY(nameEdit);
    QVERIFY(downloadEdit);
    QVERIFY(discoveryCheck);
    QVERIFY(portSpin);
    QVERIFY(restartHint);

    QCOMPARE(nameEdit->text(), QStringLiteral("old name"));
    nameEdit->setText(QStringLiteral("new name"));
    QCOMPARE(settings.displayName(), QStringLiteral("new name"));

    downloadEdit->setText(QStringLiteral("D:/Inbox"));
    QCOMPARE(settings.downloadDir(), QStringLiteral("D:/Inbox"));

    discoveryCheck->setChecked(false);
    QCOMPARE(settings.discoveryEnabled(), false);
    QVERIFY(!restartHint->isHidden());

    portSpin->setValue(9898);
    QCOMPARE(settings.listenPort(), static_cast<quint16>(9898));
    QVERIFY(!restartHint->isHidden());
}

} // namespace

QTEST_MAIN(SettingsPageTest)

#include "tst_settings_page.moc"
