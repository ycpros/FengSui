// tst_share_page.cpp
// SharePage shell state and non-mutating placeholder actions.

#include "core/ShareService.h"
#include "storage/Database.h"
#include "storage/ShareRepository.h"
#include "ui/share/SharePage.h"

#include <QCheckBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <optional>

namespace {

class SharePageTest : public QObject {
    Q_OBJECT

private slots:
    void tabsAndPlaceholderActionsAreStable();
    void realShareServiceRendersToggleAndRemove();
};

void SharePageTest::tabsAndPlaceholderActionsAreStable()
{
    FengSui::SharePage page;

    auto* tabs = page.findChild<QTabWidget*>(QStringLiteral("shareTabWidget"));
    auto* sourceList = page.findChild<QListWidget*>(QStringLiteral("shareSourceList"));
    auto* addButton = page.findChild<QPushButton*>(QStringLiteral("shareAddButton"));
    auto* hint = page.findChild<QLabel*>(QStringLiteral("shareServiceHintLabel"));

    QVERIFY(tabs);
    QCOMPARE(tabs->count(), 2);
    QCOMPARE(tabs->tabText(0), QStringLiteral("浏览共享"));
    QCOMPARE(tabs->tabText(1), QStringLiteral("我的共享"));
    QVERIFY(sourceList);
    QCOMPARE(sourceList->count(), 1);
    QVERIFY(addButton);
    QVERIFY(hint);
    QVERIFY(hint->isHidden());

    addButton->click();
    QVERIFY(!hint->isHidden());
}

void SharePageTest::realShareServiceRendersToggleAndRemove()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    FengSui::Database database(tempDir.filePath(QStringLiteral("share_page.db")));
    QVERIFY(database.initialize());

    FengSui::ShareRepository repository(&database);
    FengSui::ShareService service;
    service.setShareRepository(&repository);

    const std::optional<FengSui::SharedFolder> folder =
        service.addSharedFolder(tempDir.path(), QStringLiteral("Team Docs"));
    QVERIFY(folder.has_value());

    FengSui::SharePage page;
    page.setShareService(&service);

    auto* list = page.findChild<QListWidget*>(QStringLiteral("myShareList"));
    QVERIFY(list);
    QCOMPARE(list->count(), 1);

    auto* check = page.findChild<QCheckBox*>(
        QStringLiteral("shareActiveCheck_%1").arg(folder->shareId));
    QVERIFY(check);
    QVERIFY(check->isChecked());

    check->setChecked(false);
    QVERIFY(!service.hasActiveShares());

    check = page.findChild<QCheckBox*>(
        QStringLiteral("shareActiveCheck_%1").arg(folder->shareId));
    QVERIFY(check);
    QVERIFY(!check->isChecked());

    auto* removeButton = page.findChild<QPushButton*>(
        QStringLiteral("shareRemoveButton_%1").arg(folder->shareId));
    QVERIFY(removeButton);
    removeButton->click();

    QVERIFY(service.sharedFolders().isEmpty());
    QCOMPARE(list->count(), 1);
    QCOMPARE(list->item(0)->text(), QStringLiteral("暂无共享目录"));
}

} // namespace

QTEST_MAIN(SharePageTest)

#include "tst_share_page.moc"
