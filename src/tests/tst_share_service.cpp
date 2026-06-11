// tst_share_service.cpp
// ShareRepository / ShareService 本机共享目录管理测试。

#include "core/ShareService.h"
#include "models/SharedFolder.h"
#include "storage/Database.h"
#include "storage/ShareRepository.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <optional>

namespace {

class ShareServiceTest : public QObject {
    Q_OBJECT

private slots:
    void addListDuplicateToggleAndRemove();
    void invalidPathIsRejected();
};

void ShareServiceTest::addListDuplicateToggleAndRemove()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    FengSui::Database database(tempDir.filePath(QStringLiteral("share.db")));
    QVERIFY(database.initialize());

    FengSui::ShareRepository repository(&database);
    FengSui::ShareService service;
    service.setShareRepository(&repository);

    QSignalSpy changedSpy(&service, &FengSui::ShareService::sharedFoldersChanged);
    QSignalSpy availabilitySpy(&service,
                               &FengSui::ShareService::shareAvailabilityChanged);

    const std::optional<FengSui::SharedFolder> added =
        service.addSharedFolder(tempDir.path(), QStringLiteral("Design"));
    QVERIFY(added.has_value());
    QVERIFY(added->shareId.startsWith(QStringLiteral("sh_")));
    QCOMPARE(added->displayName, QStringLiteral("Design"));
    QVERIFY(added->isActive);
    QVERIFY(service.hasActiveShares());
    QCOMPARE(service.sharedFolders().size(), 1);
    QCOMPARE(service.activeSharedFolders().size(), 1);
    QCOMPARE(changedSpy.size(), 1);
    QCOMPARE(availabilitySpy.size(), 1);
    QCOMPARE(availabilitySpy.takeFirst().at(0).toBool(), true);

    QVERIFY(service.setSharedFolderActive(added->shareId, false));
    QVERIFY(!service.hasActiveShares());
    QCOMPARE(service.activeSharedFolders().size(), 0);
    QCOMPARE(availabilitySpy.size(), 1);
    QCOMPARE(availabilitySpy.takeFirst().at(0).toBool(), false);

    const std::optional<FengSui::SharedFolder> duplicate =
        service.addSharedFolder(tempDir.path(), QStringLiteral("Design Again"));
    QVERIFY(duplicate.has_value());
    QCOMPARE(duplicate->shareId, added->shareId);
    QCOMPARE(duplicate->displayName, QStringLiteral("Design Again"));
    QVERIFY(duplicate->isActive);
    QCOMPARE(service.sharedFolders().size(), 1);
    QVERIFY(service.hasActiveShares());
    QCOMPARE(availabilitySpy.size(), 1);
    QCOMPARE(availabilitySpy.takeFirst().at(0).toBool(), true);

    const std::optional<FengSui::SharedFolder> stored =
        repository.getSharedFolder(added->shareId);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->displayName, QStringLiteral("Design Again"));
    QVERIFY(stored->isActive);

    QVERIFY(service.removeSharedFolder(added->shareId));
    QVERIFY(service.sharedFolders().isEmpty());
    QVERIFY(!service.hasActiveShares());
    QCOMPARE(availabilitySpy.size(), 1);
    QCOMPARE(availabilitySpy.takeFirst().at(0).toBool(), false);
}

void ShareServiceTest::invalidPathIsRejected()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    FengSui::Database database(tempDir.filePath(QStringLiteral("share.db")));
    QVERIFY(database.initialize());

    FengSui::ShareRepository repository(&database);
    FengSui::ShareService service;
    service.setShareRepository(&repository);

    const std::optional<FengSui::SharedFolder> missing =
        service.addSharedFolder(tempDir.filePath(QStringLiteral("missing")));
    QVERIFY(!missing.has_value());
    QVERIFY(service.sharedFolders().isEmpty());
    QVERIFY(!service.hasActiveShares());
}

} // namespace

QTEST_MAIN(ShareServiceTest)

#include "tst_share_service.moc"
