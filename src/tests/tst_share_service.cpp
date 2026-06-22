// tst_share_service.cpp
// ShareRepository / ShareService 本机共享目录管理测试。

#include "core/ShareService.h"
#include "models/SharedFolder.h"
#include "storage/AccessGrantRepository.h"
#include "storage/Database.h"
#include "storage/ShareRepository.h"

#include <QJsonObject>
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
    void remoteShareRequestEmitsOutboundMessage();
    void firstAccessCanBeRemembered();
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

void ShareServiceTest::remoteShareRequestEmitsOutboundMessage()
{
    FengSui::ShareService service;

    FengSui::PeerInfo peer;
    peer.peerId = QStringLiteral("peer_remote");
    peer.ip = QStringLiteral("127.0.0.1");
    peer.port = 8787;

    bool emitted = false;
    QJsonObject outbound;
    QObject::connect(&service,
                     &FengSui::ShareService::outboundShareMessage,
                     &service,
                     [&](const FengSui::PeerInfo& target,
                         const QJsonObject& message) {
                         emitted = true;
                         QCOMPARE(target.peerId, peer.peerId);
                         outbound = message;
                     });

    const QString requestId = service.requestShareList(peer);
    QVERIFY(emitted);
    QVERIFY(requestId.startsWith(QStringLiteral("sr_")));
    QCOMPARE(outbound.value(QStringLiteral("type")).toString(),
             QStringLiteral("share.list"));
    QCOMPARE(outbound.value(QStringLiteral("request_id")).toString(), requestId);
}

void ShareServiceTest::firstAccessCanBeRemembered()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    FengSui::Database database(tempDir.filePath(QStringLiteral("access.db")));
    QVERIFY(database.initialize());

    FengSui::ShareRepository repository(&database);
    FengSui::AccessGrantRepository grants(&database);
    FengSui::ShareService service;
    service.setShareRepository(&repository);
    service.setAccessGrantRepository(&grants);

    const std::optional<FengSui::SharedFolder> folder =
        service.addSharedFolder(tempDir.path(), QStringLiteral("Docs"));
    QVERIFY(folder.has_value());

    QSignalSpy accessSpy(&service, &FengSui::ShareService::accessRequested);
    QJsonObject request;
    request.insert(QStringLiteral("type"), QStringLiteral("share.list"));
    request.insert(QStringLiteral("request_id"), QStringLiteral("sr_auth"));
    request.insert(QStringLiteral("from"), QStringLiteral("peer_remote"));
    request.insert(QStringLiteral("display_name"), QStringLiteral("Alice"));
    request.insert(QStringLiteral("device_name"), QStringLiteral("Laptop"));

    service.handleShareMessage(nullptr, request);
    QCOMPARE(accessSpy.size(), 1);
    QCOMPARE(accessSpy.first().at(0).toString(), QStringLiteral("sr_auth"));
    QCOMPARE(accessSpy.first().at(3).toString(), QStringLiteral("共享目录列表"));

    service.resolveAccessRequest(QStringLiteral("sr_auth"), true, true);
    QVERIFY(grants.hasGrant(QStringLiteral("peer_remote"), folder->shareId));
}

} // namespace

QTEST_MAIN(ShareServiceTest)

#include "tst_share_service.moc"
