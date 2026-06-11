// tst_main_window_navigation.cpp
// MainWindow navigation switches the stacked page without app services.

#include "ui/MainWindow.h"

#include <QListWidget>
#include <QtTest/QtTest>

namespace {

class MainWindowNavigationTest : public QObject {
    Q_OBJECT

private slots:
    void navigationSwitchesPages();
};

void MainWindowNavigationTest::navigationSwitchesPages()
{
    FengSui::MainWindow window(nullptr);

    auto* nav = window.findChild<QListWidget*>(QStringLiteral("mainNavigationList"));
    QVERIFY(nav);
    QCOMPARE(nav->count(), 5);
    QCOMPARE(window.currentPageIndex(), 0);

    nav->setCurrentRow(3);
    QCOMPARE(window.currentPageIndex(), 3);

    window.switchToPage(4);
    QCOMPARE(window.currentPageIndex(), 4);
    QCOMPARE(nav->currentRow(), 4);
}

} // namespace

QTEST_MAIN(MainWindowNavigationTest)

#include "tst_main_window_navigation.moc"
