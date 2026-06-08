// tst_protocol.cpp
// Protocol wire format 的单元测试：JSON 帧、半包、非法帧和二进制 chunk 帧。

#include "network/Protocol.h"

#include <QByteArray>
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

} // namespace

QTEST_MAIN(ProtocolTest)

#include "tst_protocol.moc"
