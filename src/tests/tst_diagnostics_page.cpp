// tst_diagnostics_page.cpp
// DiagnosticsPage renders available settings and check rows.

#include "app/AppSettings.h"
#include "models/NetworkPolicy.h"
#include "storage/Database.h"
#include "storage/SettingsRepository.h"
#include "ui/diagnostics/DiagnosticsPage.h"

#include <QLabel>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QtTest/QtTest>

namespace {

class DiagnosticsPageTest : public QObject {
    Q_OBJECT

private slots:
    void rendersSettingsAndExplicitUnimplementedChecks();
};

void DiagnosticsPageTest::rendersSettingsAndExplicitUnimplementedChecks()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    FengSui::Database database(tempDir.filePath(QStringLiteral("diagnostics.db")));
    QVERIFY(database.initialize());
    FengSui::SettingsRepository settingsRepo(&database);
    FengSui::AppSettings settings(&settingsRepo);
    QVERIFY(settings.load());
    settings.setListenPort(9090);
    settings.setAllowedCidrs(QStringLiteral("192.168.1.0/24"));

    FengSui::NetworkPolicy policy;
    policy.setAllowedCidrs({QStringLiteral("192.168.1.0/24")});

    FengSui::DiagnosticsPage page(&settings, nullptr, &policy);

    auto* portLabel = page.findChild<QLabel*>(QStringLiteral("diagnosticsPortLabel"));
    auto* ifaceTable = page.findChild<QTableWidget*>(QStringLiteral("diagnosticsInterfaceTable"));
    auto* checkTable = page.findChild<QTableWidget*>(QStringLiteral("diagnosticsCheckTable"));

    QVERIFY(portLabel);
    QVERIFY(portLabel->text().contains(QStringLiteral("9090")));
    QVERIFY(ifaceTable);
    QVERIFY(checkTable);
    QVERIFY(checkTable->rowCount() >= 5);

    bool sawUnimplemented = false;
    for (int row = 0; row < checkTable->rowCount(); ++row) {
        auto* statusItem = checkTable->item(row, 1);
        if (statusItem && statusItem->text() == QStringLiteral("未实现")) {
            sawUnimplemented = true;
        }
    }
    QVERIFY(sawUnimplemented);
}

} // namespace

QTEST_MAIN(DiagnosticsPageTest)

#include "tst_diagnostics_page.moc"
