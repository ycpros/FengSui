// tst_viewmodels.cpp
// ViewModel 层列表模型的增量行为回归测试（阶段 3-4 核心风险点）：
// - PeerListModel：upsert/remove + count
// - TransferListModel：进度增量只发对应 role、按 id 定位
// - MessageListModel：按 messageId 幂等 append（防重复气泡）+ 状态更新
// - ConversationListModel：幂等 upsert + 在线态更新

#include <QtTest>
#include <QSignalSpy>

#include "ui/viewmodels/PeerListModel.h"
#include "ui/viewmodels/TransferListModel.h"
#include "ui/viewmodels/MessageListModel.h"
#include "ui/viewmodels/ConversationListModel.h"

using namespace FengSui;

class TestViewModels : public QObject {
    Q_OBJECT

private slots:
    void peerListUpsertRemove()
    {
        PeerListModel m;
        QCOMPARE(m.rowCount(), 0);

        PeerInfo a; a.peerId = "p1"; a.displayName = "Alice"; a.online = true;
        m.upsert(a);
        QCOMPARE(m.rowCount(), 1);

        // 同 id 再 upsert：不新增行，原地更新
        a.displayName = "Alice2";
        m.upsert(a);
        QCOMPARE(m.rowCount(), 1);
        QCOMPARE(m.data(m.index(0), PeerListModel::DisplayNameRole).toString(),
                 QStringLiteral("Alice2"));

        // endpoint 组合 role
        PeerInfo b; b.peerId = "p2"; b.ip = "192.168.1.5"; b.port = 8787;
        m.upsert(b);
        QCOMPARE(m.rowCount(), 2);
        QCOMPARE(m.data(m.index(1), PeerListModel::EndpointRole).toString(),
                 QStringLiteral("192.168.1.5:8787"));

        m.remove("p1");
        QCOMPARE(m.rowCount(), 1);
        // 剩下的应还能按 id 定位（remove 后索引重建）
        m.remove("p2");
        QCOMPARE(m.rowCount(), 0);
    }

    void transferProgressIncremental()
    {
        TransferListModel m;
        TransferListModel::Row row;
        row.task.transferId = "t1";
        row.task.fileName = "a.bin";
        row.task.fileSize = 1000;
        row.task.transferredBytes = 0;
        row.task.status = TransferStatus::Transferring;
        row.peerName = "Bob";
        m.upsert(row);
        QCOMPARE(m.rowCount(), 1);

        QSignalSpy spy(&m, &QAbstractItemModel::dataChanged);
        m.updateProgress("t1", 500, 1000);

        QCOMPARE(spy.count(), 1);
        // 进度更新只应携带 Progress/Transferred/Size 三个 role
        const QList<int> roles = spy.at(0).at(2).value<QList<int>>();
        QVERIFY(roles.contains(TransferListModel::ProgressRole));
        QVERIFY(roles.contains(TransferListModel::TransferredRole));
        QVERIFY(!roles.contains(TransferListModel::FileNameRole));

        QCOMPARE(m.data(m.index(0), TransferListModel::ProgressRole).toDouble(), 0.5);

        // 状态更新
        m.updateStatus("t1", TransferStatus::Completed);
        QCOMPARE(m.data(m.index(0), TransferListModel::StatusRole).toInt(),
                 static_cast<int>(TransferStatus::Completed));
    }

    void messageIdempotentAppend()
    {
        MessageListModel m;
        m.setLocalPeerId("me");

        Message msg; msg.messageId = "m1"; msg.senderId = "me"; msg.content = "hi";
        msg.status = MessageStatus::Sending;
        m.append(msg);
        QCOMPARE(m.rowCount(), 1);
        QCOMPARE(m.data(m.index(0), MessageListModel::IsOutgoingRole).toBool(), true);

        // 同 messageId 再 append：不产生重复气泡（防 messageStored 重复触发）
        msg.status = MessageStatus::Sent;
        m.append(msg);
        QCOMPARE(m.rowCount(), 1);

        // 状态增量更新
        QSignalSpy spy(&m, &QAbstractItemModel::dataChanged);
        m.updateStatus("m1", static_cast<int>(MessageStatus::Delivered));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(m.data(m.index(0), MessageListModel::StatusRole).toInt(),
                 static_cast<int>(MessageStatus::Delivered));

        // 对方消息 isOutgoing=false
        Message in; in.messageId = "m2"; in.senderId = "peer"; in.content = "yo";
        m.append(in);
        QCOMPARE(m.data(m.index(1), MessageListModel::IsOutgoingRole).toBool(), false);
    }

    void conversationIdempotentAndOnline()
    {
        ConversationListModel m;
        ConversationListModel::Row row;
        row.conv.conversationId = "c1";
        row.conv.peerId = "p1";
        row.conv.lastMessage = "hello";
        row.displayName = "Alice";
        row.online = false;
        m.upsert(row);
        QCOMPARE(m.rowCount(), 1);

        // 幂等 upsert
        row.conv.lastMessage = "hello2";
        m.upsert(row);
        QCOMPARE(m.rowCount(), 1);
        QCOMPARE(m.data(m.index(0), ConversationListModel::LastMessageRole).toString(),
                 QStringLiteral("hello2"));

        // 在线态按 peerId 更新
        QSignalSpy spy(&m, &QAbstractItemModel::dataChanged);
        m.setOnline("p1", true);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(m.data(m.index(0), ConversationListModel::OnlineRole).toBool(), true);
    }
};

QTEST_MAIN(TestViewModels)
#include "tst_viewmodels.moc"
