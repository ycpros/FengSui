// tst_transfer_center_page.cpp
// 传输中心页面测试：历史任务渲染、状态过滤和实时刷新。

#include "app/AppSettings.h"
#include "core/CourierService.h"
#include "models/TransferTask.h"
#include "storage/Database.h"
#include "storage/SettingsRepository.h"
#include "storage/TransferRepository.h"
#include "ui/transfer_center/TransferCenterPage.h"

#include <QComboBox>
#include <QFile>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QtTest/QtTest>

namespace {

using namespace FengSui;

class TransferCenterPageTest : public QObject {
    Q_OBJECT

private slots:
    void emptyStateIsShownWithoutTasks();
    void rendersTasksAndFiltersByStatus();
    void refreshesWhenCourierSignalsChange();

private:
    static TransferTask makeTask(const QString& id,
                                 TransferStatus status,
                                 TransferDirection direction = TransferDirection::Upload);
    static void selectFilter(QComboBox* combo, const QString& label);
};

TransferTask TransferCenterPageTest::makeTask(const QString& id,
                                              TransferStatus status,
                                              TransferDirection direction)
{
    TransferTask task;
    task.transferId = id;
    task.direction = direction;
    task.peerId = QStringLiteral("peer_remote");
    task.fileName = QStringLiteral("%1.txt").arg(id);
    task.filePath = QStringLiteral("C:/tmp/%1.txt").arg(id);
    task.fileSize = 100;
    task.transferredBytes = status == TransferStatus::Completed ? 100 : 25;
    task.status = status;
    task.errorMessage = (status == TransferStatus::Failed
                         || status == TransferStatus::Rejected
                         || status == TransferStatus::Cancelled)
        ? QStringLiteral("test reason")
        : QString();
    task.createdAt = QDateTime::currentDateTimeUtc();
    return task;
}

void TransferCenterPageTest::selectFilter(QComboBox* combo, const QString& label)
{
    const int index = combo->findText(label);
    QVERIFY(index >= 0);
    combo->setCurrentIndex(index);
}

void TransferCenterPageTest::emptyStateIsShownWithoutTasks()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    Database database(tempDir.filePath(QStringLiteral("empty.db")));
    QVERIFY(database.initialize());
    SettingsRepository settingsRepo(&database);
    AppSettings settings(&settingsRepo);
    QVERIFY(settings.load());
    TransferRepository transfers(&database);
    CourierService courier;
    courier.setAppSettings(&settings);
    courier.setTransferRepository(&transfers);

    TransferCenterPage page;
    page.setCourierService(&courier);

    auto* table = page.findChild<QTableWidget*>(QStringLiteral("transferTable"));
    auto* emptyLabel = page.findChild<QLabel*>(QStringLiteral("transferEmptyLabel"));
    QVERIFY(table);
    QVERIFY(emptyLabel);
    QCOMPARE(table->rowCount(), 0);
    QVERIFY(table->isHidden());
    QVERIFY(!emptyLabel->isHidden());
}

void TransferCenterPageTest::rendersTasksAndFiltersByStatus()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    Database database(tempDir.filePath(QStringLiteral("tasks.db")));
    QVERIFY(database.initialize());
    SettingsRepository settingsRepo(&database);
    AppSettings settings(&settingsRepo);
    QVERIFY(settings.load());
    TransferRepository transfers(&database);

    TransferTask completed = makeTask(QStringLiteral("completed"),
                                      TransferStatus::Completed,
                                      TransferDirection::Download);
    completed.filePath = tempDir.filePath(QStringLiteral("completed.txt"));
    QFile completedFile(completed.filePath);
    QVERIFY(completedFile.open(QIODevice::WriteOnly));
    QVERIFY(completedFile.write("done") == 4);
    completedFile.close();

    QVERIFY(transfers.saveTask(completed));
    QVERIFY(transfers.saveTask(makeTask(QStringLiteral("failed"),
                                        TransferStatus::Failed)));
    QVERIFY(transfers.saveTask(makeTask(QStringLiteral("rejected"),
                                        TransferStatus::Rejected)));
    QVERIFY(transfers.saveTask(makeTask(QStringLiteral("moving"),
                                        TransferStatus::Transferring)));

    CourierService courier;
    courier.setAppSettings(&settings);
    courier.setTransferRepository(&transfers);

    TransferCenterPage page;
    page.setCourierService(&courier);

    auto* table = page.findChild<QTableWidget*>(QStringLiteral("transferTable"));
    auto* combo = page.findChild<QComboBox*>(QStringLiteral("transferFilterCombo"));
    QVERIFY(table);
    QVERIFY(combo);
    QCOMPARE(table->rowCount(), 4);

    selectFilter(combo, QStringLiteral("已完成"));
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 3)->text(), QStringLiteral("已完成"));

    auto* button = page.findChild<QPushButton*>(
        QStringLiteral("openTransferDir_completed"));
    QVERIFY(button);
    QVERIFY(button->isEnabled());

    selectFilter(combo, QStringLiteral("失败"));
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 3)->text(), QStringLiteral("失败"));
    QCOMPARE(table->item(0, 7)->text(), QStringLiteral("test reason"));

    selectFilter(combo, QStringLiteral("全部"));
    QCOMPARE(table->rowCount(), 4);
}

void TransferCenterPageTest::refreshesWhenCourierSignalsChange()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    Database database(tempDir.filePath(QStringLiteral("refresh.db")));
    QVERIFY(database.initialize());
    SettingsRepository settingsRepo(&database);
    AppSettings settings(&settingsRepo);
    QVERIFY(settings.load());
    TransferRepository transfers(&database);

    TransferTask task = makeTask(QStringLiteral("live"),
                                 TransferStatus::Waiting);
    task.transferredBytes = 0;
    QVERIFY(transfers.saveTask(task));

    CourierService courier;
    courier.setAppSettings(&settings);
    courier.setTransferRepository(&transfers);

    TransferCenterPage page;
    page.setCourierService(&courier);

    auto* table = page.findChild<QTableWidget*>(QStringLiteral("transferTable"));
    QVERIFY(table);
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 3)->text(), QStringLiteral("等待中"));

    QVERIFY(transfers.updateProgress(task.transferId, 50));
    emit courier.transferProgress(task.transferId, 50, 100);

    QTRY_COMPARE(table->item(0, 3)->text(), QStringLiteral("传输中"));
    auto* progressCell = table->cellWidget(0, 4);
    QVERIFY(progressCell);
    auto* progress = progressCell->findChild<QProgressBar*>(
        QStringLiteral("transferProgressBar_live"));
    QVERIFY(progress);
    QCOMPARE(progress->value(), 50);

    QVERIFY(transfers.updateStatus(task.transferId,
                                   TransferStatus::Failed,
                                   QStringLiteral("network down")));
    emit courier.transferFailed(task.transferId, QStringLiteral("network down"));

    QTRY_COMPARE(table->item(0, 3)->text(), QStringLiteral("失败"));
    QCOMPARE(table->item(0, 7)->text(), QStringLiteral("network down"));
}

} // namespace

QTEST_MAIN(TransferCenterPageTest)

#include "tst_transfer_center_page.moc"
