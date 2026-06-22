// tst_protocol.cpp
// Protocol wire format 的单元测试：JSON 帧、半包、非法帧和二进制 chunk 帧。

#include "network/Protocol.h"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QtEndian>
#include <QtTest/QtTest>

namespace {

class ProtocolTest : public QObject {
    Q_OBJECT

private slots:
    void packExtractUnpackRoundTrip();
    void extractFrameWaitsForPartialPayload();
    void extractFrameRejectsOversizedPayload();
    void chunkFrameRoundTrip();
    void shareProtocolBuilders();
};

void ProtocolTest::packExtractUnpackRoundTrip()
{
    const QJsonObject message = FengSui::Protocol::buildTextMessage(
        QStringLiteral("msg_001"),
        QStringLiteral("peer_a"),
        QStringLiteral("peer_b"),
        QStringLiteral("hello"));

    QByteArray buffer = FengSui::Protocol::pack(message);
    QVERIFY(!buffer.isEmpty());

    QByteArray frame;
    QString error;
    QVERIFY(FengSui::Protocol::extractFrame(buffer, frame, error));
    QVERIFY(error.isEmpty());
    QVERIFY(buffer.isEmpty());

    QJsonObject unpacked;
    QVERIFY(FengSui::Protocol::unpack(frame, unpacked, error));
    QCOMPARE(FengSui::Protocol::messageType(unpacked), QStringLiteral("message.text"));
    QCOMPARE(unpacked.value(QStringLiteral("message_id")).toString(),
             QStringLiteral("msg_001"));
    QCOMPARE(unpacked.value(QStringLiteral("content")).toString(),
             QStringLiteral("hello"));
}

void ProtocolTest::extractFrameWaitsForPartialPayload()
{
    const QJsonObject message = FengSui::Protocol::buildAck(
        QStringLiteral("msg_001"),
        QStringLiteral("delivered"));
    QByteArray buffer = FengSui::Protocol::pack(message);
    buffer.chop(1);

    QByteArray frame;
    QString error;
    QVERIFY(!FengSui::Protocol::extractFrame(buffer, frame, error));
    QVERIFY(error.isEmpty());
    QVERIFY(frame.isEmpty());
}

void ProtocolTest::extractFrameRejectsOversizedPayload()
{
    QByteArray buffer;
    buffer.resize(static_cast<int>(sizeof(quint32)));
    qToBigEndian<quint32>(101 * 1024 * 1024,
                          reinterpret_cast<uchar*>(buffer.data()));

    QByteArray frame;
    QString error;
    QVERIFY(!FengSui::Protocol::extractFrame(buffer, frame, error));
    QVERIFY(!error.isEmpty());
}

void ProtocolTest::chunkFrameRoundTrip()
{
    const QByteArray payload("chunk-payload");
    const QByteArray frame = FengSui::Protocol::buildChunkFrame(
        QStringLiteral("tr_001"),
        7,
        payload);

    QString transferId;
    quint32 chunkIndex = 0;
    QByteArray parsedPayload;
    QString error;
    QVERIFY(FengSui::Protocol::parseChunkFrame(frame,
                                               transferId,
                                               chunkIndex,
                                               parsedPayload,
                                               error));
    QCOMPARE(transferId, QStringLiteral("tr_001"));
    QCOMPARE(chunkIndex, 7U);
    QCOMPARE(parsedPayload, payload);
    QVERIFY(FengSui::Protocol::isLikelyChunkFrame(frame));
}

void ProtocolTest::shareProtocolBuilders()
{
    const QJsonObject list = FengSui::Protocol::buildShareListRequest(
        QStringLiteral("sr_001"),
        QStringLiteral("peer_a"),
        QStringLiteral("peer_b"));
    QVERIFY(FengSui::Protocol::isShareMessage(list));
    QCOMPARE(FengSui::Protocol::messageType(list), QStringLiteral("share.list"));
    QCOMPARE(list.value(QStringLiteral("request_id")).toString(),
             QStringLiteral("sr_001"));

    QJsonArray shares;
    QJsonObject share;
    share.insert(QStringLiteral("share_id"), QStringLiteral("sh_001"));
    share.insert(QStringLiteral("display_name"), QStringLiteral("Design"));
    share.insert(QStringLiteral("file_count"), 2);
    shares.append(share);
    const QJsonObject listReply = FengSui::Protocol::buildShareListReply(
        QStringLiteral("sr_001"),
        QStringLiteral("peer_b"),
        QStringLiteral("peer_a"),
        shares);
    QVERIFY(FengSui::Protocol::isShareMessage(listReply));
    QCOMPARE(listReply.value(QStringLiteral("shares")).toArray().size(), 1);

    const QJsonObject items = FengSui::Protocol::buildShareItemsRequest(
        QStringLiteral("sr_002"),
        QStringLiteral("peer_a"),
        QStringLiteral("peer_b"),
        QStringLiteral("sh_001"),
        QStringLiteral("/docs"));
    QCOMPARE(FengSui::Protocol::messageType(items), QStringLiteral("share.items"));
    QCOMPARE(items.value(QStringLiteral("path")).toString(), QStringLiteral("/docs"));

    const QJsonObject download = FengSui::Protocol::buildShareDownloadRequest(
        QStringLiteral("sd_001"),
        QStringLiteral("peer_a"),
        QStringLiteral("peer_b"),
        QStringLiteral("sh_001"),
        QStringLiteral("/docs/a.txt"));
    QVERIFY(FengSui::Protocol::isShareMessage(download));
    QCOMPARE(download.value(QStringLiteral("download_id")).toString(),
             QStringLiteral("sd_001"));

    const QJsonObject error = FengSui::Protocol::buildShareError(
        QStringLiteral("sd_001"),
        QStringLiteral("peer_b"),
        QStringLiteral("peer_a"),
        QStringLiteral("ACCESS_DENIED"),
        QStringLiteral("访问被拒绝"));
    QCOMPARE(FengSui::Protocol::messageType(error), QStringLiteral("share.error"));
    QCOMPARE(error.value(QStringLiteral("error_code")).toString(),
             QStringLiteral("ACCESS_DENIED"));
}

} // namespace

QTEST_MAIN(ProtocolTest)

#include "tst_protocol.moc"
