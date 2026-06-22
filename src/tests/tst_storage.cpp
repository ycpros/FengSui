// tst_storage.cpp
// SQLite 仓储测试：建表、peer 同步、会话创建、消息保存和状态更新。

#include "models/AccessGrant.h"
#include "models/DownloadLog.h"
#include "models/Message.h"
#include "models/PeerInfo.h"
#include "models/SharedFolder.h"
#include "storage/AccessGrantRepository.h"
#include "storage/ConversationRepository.h"
#include "storage/Database.h"
#include "storage/DownloadLogRepository.h"
#include "storage/ManualPeerRepository.h"
#include "storage/MessageRepository.h"
#include "storage/ShareRepository.h"

#include <QDateTime>
#include <QTemporaryDir>
#include <QtTest/QtTest>

namespace {

class StorageTest : public QObject {
    Q_OBJECT

private slots:
    void conversationAndMessageRoundTrip();
    void accessGrantAndDownloadLogRoundTrip();
};

void StorageTest::conversationAndMessageRoundTrip()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    FengSui::Database database(tempDir.filePath(QStringLiteral("storage.db")));
    QVERIFY(database.initialize());

    FengSui::ConversationRepository conversations(&database,
                                                   QStringLiteral("peer_local"));
    FengSui::MessageRepository messages(&database);
    FengSui::ManualPeerRepository manualPeers(&database);

    FengSui::PeerInfo peer;
    peer.peerId = QStringLiteral("peer_remote");
    peer.displayName = QStringLiteral("Remote");
    peer.deviceName = QStringLiteral("Remote-PC");
    peer.ip = QStringLiteral("127.0.0.1");
    peer.port = 18787;
    peer.os = QStringLiteral("windows");
    peer.online = true;
    peer.lastSeenAt = QDateTime::currentDateTimeUtc();
    peer.version = QStringLiteral("0.1.0");

    QVERIFY(conversations.ensurePeer(peer));
    const std::optional<FengSui::PeerInfo> storedPeer =
        conversations.getPeer(peer.peerId);
    QVERIFY(storedPeer.has_value());
    QCOMPARE(storedPeer->ip, peer.ip);
    QCOMPARE(storedPeer->port, peer.port);

    const FengSui::Conversation conversation =
        conversations.createOrGetConversation(peer.peerId);
    QVERIFY(!conversation.conversationId.isEmpty());
    QCOMPARE(conversation.peerId, peer.peerId);

    FengSui::Message message;
    message.messageId = QStringLiteral("msg_storage_001");
    message.conversationId = conversation.conversationId;
    message.senderId = QStringLiteral("peer_local");
    message.type = FengSui::MessageType::Text;
    message.content = QStringLiteral("hello storage");
    message.createdAt = QDateTime::currentDateTimeUtc();
    message.status = FengSui::MessageStatus::Sending;

    QVERIFY(messages.saveMessage(message));
    QList<FengSui::Message> history =
        messages.getMessages(conversation.conversationId);
    QCOMPARE(history.size(), 1);
    QCOMPARE(history.first().content, message.content);
    QCOMPARE(history.first().status, FengSui::MessageStatus::Sending);

    QVERIFY(messages.updateMessageStatus(message.messageId,
                                         FengSui::MessageStatus::Delivered));
    const std::optional<FengSui::Message> delivered =
        messages.getMessage(message.messageId);
    QVERIFY(delivered.has_value());
    QCOMPARE(delivered->status, FengSui::MessageStatus::Delivered);

    QVERIFY(conversations.updateLastMessage(conversation.conversationId,
                                            message.content,
                                            message.createdAt));
    QVERIFY(conversations.incrementUnreadCount(conversation.conversationId));
    QVERIFY(conversations.resetUnreadCount(conversation.conversationId));

    FengSui::ManualPeer manualPeer;
    manualPeer.name = QStringLiteral("Manual Remote");
    manualPeer.host = QStringLiteral("192.168.10.33");
    manualPeer.port = 8787;
    manualPeer.lastSuccessAt = QDateTime::currentDateTimeUtc();
    QVERIFY(manualPeers.upsertManualPeer(manualPeer));

    const std::optional<FengSui::ManualPeer> storedManualPeer =
        manualPeers.getManualPeer(manualPeer.host, manualPeer.port);
    QVERIFY(storedManualPeer.has_value());
    QCOMPARE(storedManualPeer->name, manualPeer.name);
    QCOMPARE(storedManualPeer->host, manualPeer.host);
    QCOMPARE(storedManualPeer->port, manualPeer.port);
    QCOMPARE(manualPeers.getAllManualPeers().size(), 1);
}

void StorageTest::accessGrantAndDownloadLogRoundTrip()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    FengSui::Database database(tempDir.filePath(QStringLiteral("share_storage.db")));
    QVERIFY(database.initialize());

    FengSui::ShareRepository shares(&database);
    FengSui::AccessGrantRepository grants(&database);
    FengSui::DownloadLogRepository logs(&database);

    FengSui::SharedFolder folder;
    folder.shareId = QStringLiteral("sh_test");
    folder.localPath = tempDir.path();
    folder.displayName = QStringLiteral("Docs");
    folder.isActive = true;
    QVERIFY(shares.saveSharedFolder(folder));

    FengSui::AccessGrant grant;
    grant.peerId = QStringLiteral("peer_remote");
    grant.shareId = folder.shareId;
    grant.grantedAt = QDateTime::currentDateTimeUtc();
    grant.remember = true;
    QVERIFY(grants.saveGrant(grant));
    QVERIFY(grants.hasGrant(grant.peerId, grant.shareId));

    const std::optional<FengSui::AccessGrant> storedGrant =
        grants.grantFor(grant.peerId, grant.shareId);
    QVERIFY(storedGrant.has_value());
    QVERIFY(storedGrant->remember);

    FengSui::DownloadLog log;
    log.logId = QStringLiteral("dl_test");
    log.peerId = grant.peerId;
    log.shareId = folder.shareId;
    log.remotePath = QStringLiteral("/a.txt");
    log.localPath = tempDir.filePath(QStringLiteral("a.txt"));
    log.fileName = QStringLiteral("a.txt");
    log.fileSize = 12;
    log.downloadedAt = QDateTime::currentDateTimeUtc();
    log.success = true;
    QVERIFY(logs.saveLog(log));

    const QList<FengSui::DownloadLog> recent = logs.recentLogs();
    QCOMPARE(recent.size(), 1);
    QCOMPARE(recent.first().logId, log.logId);
    QCOMPARE(recent.first().remotePath, log.remotePath);
    QVERIFY(recent.first().success);
}

} // namespace

QTEST_MAIN(StorageTest)

#include "tst_storage.moc"
