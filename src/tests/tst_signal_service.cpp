// tst_signal_service.cpp
// SignalService 集成测试：两个 localhost 服务间发送首条文本消息并收到 ACK。

#include "app/AppSettings.h"
#include "core/SignalService.h"
#include "models/Message.h"
#include "models/NetworkPolicy.h"
#include "models/PeerInfo.h"
#include "network/TcpServer.h"
#include "storage/ConversationRepository.h"
#include "storage/Database.h"
#include "storage/MessageRepository.h"
#include "storage/SettingsRepository.h"

#include <QDateTime>
#include <QHostAddress>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QtTest/QtTest>

namespace {

class SignalServiceTest : public QObject {
    Q_OBJECT

private slots:
    void firstTextMessageIsQueuedDeliveredAndPersisted();
    void tcpServerStartsWhenAtLeastOneEndpointBinds();
    void tcpServerFailsWithNoEndpoints();
    void signalServiceFailsWithoutAuthorizedPolicy();

private:
    static quint16 reserveLocalPort();
    static bool hasMessageStatus(FengSui::MessageRepository* repository,
                                 const QString& messageId,
                                 FengSui::MessageStatus status);
};

quint16 SignalServiceTest::reserveLocalPort()
{
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, 0)) {
        return 0;
    }
    const quint16 port = server.serverPort();
    server.close();
    return port;
}

bool SignalServiceTest::hasMessageStatus(FengSui::MessageRepository* repository,
                                         const QString& messageId,
                                         FengSui::MessageStatus status)
{
    const std::optional<FengSui::Message> message =
        repository->getMessage(messageId);
    return message.has_value() && message->status == status;
}

void SignalServiceTest::firstTextMessageIsQueuedDeliveredAndPersisted()
{
    QTemporaryDir tempDirA;
    QTemporaryDir tempDirB;
    QVERIFY(tempDirA.isValid());
    QVERIFY(tempDirB.isValid());

    const quint16 portA = reserveLocalPort();
    const quint16 portB = reserveLocalPort();
    QVERIFY(portA != 0);
    QVERIFY(portB != 0);
    QVERIFY(portA != portB);

    FengSui::Database databaseA(tempDirA.filePath(QStringLiteral("a.db")));
    FengSui::Database databaseB(tempDirB.filePath(QStringLiteral("b.db")));
    QVERIFY(databaseA.initialize());
    QVERIFY(databaseB.initialize());

    FengSui::SettingsRepository settingsRepoA(&databaseA);
    FengSui::SettingsRepository settingsRepoB(&databaseB);
    FengSui::AppSettings settingsA(&settingsRepoA);
    FengSui::AppSettings settingsB(&settingsRepoB);
    QVERIFY(settingsA.load());
    QVERIFY(settingsB.load());
    settingsA.setPeerId(QStringLiteral("peer_a"));
    settingsB.setPeerId(QStringLiteral("peer_b"));
    settingsA.setListenPort(portA);
    settingsB.setListenPort(portB);

    FengSui::ConversationRepository conversationRepoA(&databaseA,
                                                      settingsA.peerId());
    FengSui::ConversationRepository conversationRepoB(&databaseB,
                                                      settingsB.peerId());
    FengSui::MessageRepository messageRepoA(&databaseA);
    FengSui::MessageRepository messageRepoB(&databaseB);

    FengSui::SignalService serviceA(&settingsA);
    FengSui::SignalService serviceB(&settingsB);
    serviceA.setConversationRepository(&conversationRepoA);
    serviceA.setMessageRepository(&messageRepoA);
    serviceB.setConversationRepository(&conversationRepoB);
    serviceB.setMessageRepository(&messageRepoB);

    QString error;
    QVERIFY2(serviceA.start(QHostAddress::LocalHost, error),
             qPrintable(error));
    QVERIFY2(serviceB.start(QHostAddress::LocalHost, error),
             qPrintable(error));

    FengSui::PeerInfo peerB;
    peerB.peerId = settingsB.peerId();
    peerB.displayName = QStringLiteral("Peer B");
    peerB.deviceName = QStringLiteral("B-PC");
    peerB.ip = QStringLiteral("127.0.0.1");
    peerB.port = portB;
    peerB.os = QStringLiteral("windows");
    peerB.online = true;
    peerB.lastSeenAt = QDateTime::currentDateTimeUtc();
    peerB.version = QStringLiteral("0.1.0");

    const FengSui::Conversation conversation =
        serviceA.openConversation(peerB);
    QVERIFY(!conversation.conversationId.isEmpty());

    const QString messageId =
        serviceA.sendMessage(peerB, QStringLiteral("first hello"));
    QVERIFY(!messageId.isEmpty());

    QTRY_VERIFY_WITH_TIMEOUT(
        hasMessageStatus(&messageRepoA,
                         messageId,
                         FengSui::MessageStatus::Delivered),
        5000);

    QTRY_VERIFY_WITH_TIMEOUT(messageRepoB.getMessage(messageId).has_value(),
                             5000);
    const std::optional<FengSui::Message> received =
        messageRepoB.getMessage(messageId);
    QVERIFY(received.has_value());
    QCOMPARE(received->content, QStringLiteral("first hello"));
    QCOMPARE(received->senderId, settingsA.peerId());
    QCOMPARE(received->status, FengSui::MessageStatus::Delivered);

    const std::optional<FengSui::PeerInfo> storedPeerB =
        serviceA.peerForConversation(conversation.conversationId);
    QVERIFY(storedPeerB.has_value());
    QCOMPARE(storedPeerB->ip, peerB.ip);
    QCOMPARE(storedPeerB->port, peerB.port);
}

void SignalServiceTest::tcpServerStartsWhenAtLeastOneEndpointBinds()
{
    const quint16 blockedPort = reserveLocalPort();
    QVERIFY(blockedPort != 0);

    QTcpServer blocker;
    QVERIFY(blocker.listen(QHostAddress::LocalHost, blockedPort));

    FengSui::BindEndpoint blocked;
    blocked.interfaceId = QStringLiteral("lo-blocked");
    blocked.interfaceName = QStringLiteral("Loopback blocked");
    blocked.ip = QHostAddress::LocalHost;
    blocked.port = blockedPort;

    FengSui::BindEndpoint available;
    available.interfaceId = QStringLiteral("lo-available");
    available.interfaceName = QStringLiteral("Loopback available");
    available.ip = QHostAddress::LocalHost;
    available.port = 0;

    FengSui::TcpServer server;
    QString error;
    QVERIFY2(server.start(QList<FengSui::BindEndpoint>{blocked, available}, error),
             qPrintable(error));
    QCOMPARE(server.boundEndpoints().size(), 1);
    QCOMPARE(server.boundEndpoints().first().interfaceId,
             QStringLiteral("lo-available"));
    server.stop();
}

void SignalServiceTest::tcpServerFailsWithNoEndpoints()
{
    FengSui::TcpServer server;
    QString error;
    QVERIFY(!server.start(QList<FengSui::BindEndpoint>{}, error));
    QVERIFY(!error.isEmpty());
}

void SignalServiceTest::signalServiceFailsWithoutAuthorizedPolicy()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    FengSui::Database database(tempDir.filePath(QStringLiteral("policy.db")));
    QVERIFY(database.initialize());
    FengSui::SettingsRepository settingsRepo(&database);
    FengSui::AppSettings settings(&settingsRepo);
    QVERIFY(settings.load());
    settings.setPeerId(QStringLiteral("peer_policy"));

    FengSui::SignalService service(&settings);
    QString error;
    QVERIFY(!service.start(error));
    QVERIFY(error.contains(QStringLiteral("authorized")));
}

} // namespace

QTEST_MAIN(SignalServiceTest)

#include "tst_signal_service.moc"
