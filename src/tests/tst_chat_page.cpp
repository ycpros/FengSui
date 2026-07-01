// tst_chat_page.cpp
// ChatPage regression tests for conversation-list refresh/selection behavior.

#include "core/SignalService.h"
#include "models/PeerInfo.h"
#include "storage/ConversationRepository.h"
#include "storage/Database.h"
#include "storage/MessageRepository.h"
#include "ui/chat/ChatPage.h"

#include <QDateTime>
#include <QListWidget>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

namespace {

FengSui::PeerInfo makePeer(const QString& peerId,
                           const QString& displayName,
                           quint16 port)
{
    FengSui::PeerInfo peer;
    peer.peerId = peerId;
    peer.displayName = displayName;
    peer.deviceName = displayName;
    peer.ip = QStringLiteral("127.0.0.1");
    peer.port = port;
    peer.os = QStringLiteral("windows");
    peer.online = true;
    peer.lastSeenAt = QDateTime::currentDateTimeUtc();
    peer.version = QStringLiteral("0.1.0");
    return peer;
}

class ChatPageTest : public QObject {
    Q_OBJECT

private slots:
    void selectingConversationDoesNotRecurseDuringRefresh();
};

void ChatPageTest::selectingConversationDoesNotRecurseDuringRefresh()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    FengSui::Database database(tempDir.filePath(QStringLiteral("chat_page.db")));
    QVERIFY(database.initialize());

    FengSui::ConversationRepository conversations(&database,
                                                   QStringLiteral("peer_local"));
    FengSui::MessageRepository messages(&database);

    const FengSui::PeerInfo firstPeer =
        makePeer(QStringLiteral("peer_alpha"), QStringLiteral("Alpha"), 18787);
    const FengSui::PeerInfo secondPeer =
        makePeer(QStringLiteral("peer_beta"), QStringLiteral("Beta"), 18788);

    QVERIFY(conversations.ensurePeer(firstPeer));
    QVERIFY(conversations.ensurePeer(secondPeer));

    const FengSui::Conversation firstConversation =
        conversations.createOrGetConversation(firstPeer.peerId);
    const FengSui::Conversation secondConversation =
        conversations.createOrGetConversation(secondPeer.peerId);
    QVERIFY(!firstConversation.conversationId.isEmpty());
    QVERIFY(!secondConversation.conversationId.isEmpty());
    QVERIFY(conversations.updateLastMessage(firstConversation.conversationId,
                                            QStringLiteral("first"),
                                            QDateTime::currentDateTimeUtc().addSecs(-1)));
    QVERIFY(conversations.updateLastMessage(secondConversation.conversationId,
                                            QStringLiteral("second"),
                                            QDateTime::currentDateTimeUtc()));

    FengSui::SignalService signalService(nullptr, nullptr);
    signalService.setConversationRepository(&conversations);
    signalService.setMessageRepository(&messages);

    FengSui::ChatPage page;
    page.setLocalPeerId(QStringLiteral("peer_local"));
    page.setSignalService(&signalService);

    auto* list = page.findChild<QListWidget*>(QStringLiteral("conversationList"));
    QVERIFY(list);
    QCOMPARE(list->count(), 2);

    list->setCurrentRow(0);
    QVERIFY(list->currentItem());
    const QString selectedConversationId =
        list->currentItem()->data(Qt::UserRole).toString();
    QVERIFY(!selectedConversationId.isEmpty());

    QSignalSpy selectionSpy(list, &QListWidget::currentItemChanged);
    page.refreshConversationList();

    QCOMPARE(selectionSpy.count(), 0);
    QVERIFY(list->currentItem());
    QCOMPARE(list->currentItem()->data(Qt::UserRole).toString(),
             selectedConversationId);
}

} // namespace

QTEST_MAIN(ChatPageTest)

#include "tst_chat_page.moc"
