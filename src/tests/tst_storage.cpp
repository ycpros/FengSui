// tst_storage.cpp
// SQLite 仓储测试：建表、peer 同步、会话创建、消息保存和状态更新。

#include "models/Message.h"
#include "models/PeerInfo.h"
#include "storage/ConversationRepository.h"
#include "storage/Database.h"
#include "storage/MessageRepository.h"

#include <QDateTime>
#include <QTemporaryDir>
#include <QtTest/QtTest>

namespace {

class StorageTest : public QObject {
    Q_OBJECT

private slots:
    void conversationAndMessageRoundTrip();
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
}

} // namespace

QTEST_MAIN(StorageTest)

#include "tst_storage.moc"
