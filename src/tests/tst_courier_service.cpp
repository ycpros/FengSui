// tst_courier_service.cpp
// CourierService 集成测试：本地双节点单文件传输、拒绝与接收端失败路径。

#include "app/AppSettings.h"
#include "core/CourierService.h"
#include "core/SignalService.h"
#include "models/PeerInfo.h"
#include "models/TransferTask.h"
#include "storage/Database.h"
#include "storage/SettingsRepository.h"
#include "storage/TransferRepository.h"

#include <QFile>
#include <QHostAddress>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QtTest/QtTest>

namespace {

class CourierServiceTest : public QObject {
    Q_OBJECT

private slots:
    void acceptedFileTransferCompletesAndPersists();
    void rejectedTransferMarksBothSidesRejected();
    void invalidDownloadDirectoryFailsBothSides();

private:
    static quint16 reserveLocalPort();
    static bool writeFile(const QString& path, const QByteArray& data);
    static QByteArray readFile(const QString& path);
    static bool taskHasStatus(FengSui::TransferRepository* repository,
                              const QString& transferId,
                              FengSui::TransferStatus status);
    static FengSui::PeerInfo peerFor(const FengSui::AppSettings& settings,
                                     quint16 port);
};

quint16 CourierServiceTest::reserveLocalPort()
{
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, 0)) {
        return 0;
    }
    const quint16 port = server.serverPort();
    server.close();
    return port;
}

bool CourierServiceTest::writeFile(const QString& path, const QByteArray& data)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    return file.write(data) == data.size();
}

QByteArray CourierServiceTest::readFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
}

bool CourierServiceTest::taskHasStatus(FengSui::TransferRepository* repository,
                                       const QString& transferId,
                                       FengSui::TransferStatus status)
{
    const std::optional<FengSui::TransferTask> task =
        repository->getTask(transferId);
    return task.has_value() && task->status == status;
}

FengSui::PeerInfo CourierServiceTest::peerFor(const FengSui::AppSettings& settings,
                                              quint16 port)
{
    FengSui::PeerInfo peer;
    peer.peerId = settings.peerId();
    peer.displayName = settings.displayName();
    peer.deviceName = QStringLiteral("Test-PC");
    peer.ip = QStringLiteral("127.0.0.1");
    peer.port = port;
    peer.online = true;
    return peer;
}

void CourierServiceTest::acceptedFileTransferCompletesAndPersists()
{
    QTemporaryDir tempA;
    QTemporaryDir tempB;
    QVERIFY(tempA.isValid());
    QVERIFY(tempB.isValid());

    const quint16 portA = reserveLocalPort();
    const quint16 portB = reserveLocalPort();
    QVERIFY(portA != 0);
    QVERIFY(portB != 0);

    FengSui::Database databaseA(tempA.filePath(QStringLiteral("a.db")));
    FengSui::Database databaseB(tempB.filePath(QStringLiteral("b.db")));
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
    settingsA.setDownloadDir(tempA.filePath(QStringLiteral("downloads")));
    settingsB.setDownloadDir(tempB.filePath(QStringLiteral("downloads")));

    FengSui::TransferRepository transfersA(&databaseA);
    FengSui::TransferRepository transfersB(&databaseB);
    FengSui::SignalService signalA(&settingsA);
    FengSui::SignalService signalB(&settingsB);
    FengSui::CourierService courierA;
    FengSui::CourierService courierB;
    courierA.setSignalService(&signalA);
    courierA.setAppSettings(&settingsA);
    courierA.setTransferRepository(&transfersA);
    courierA.setLocalPeerId(settingsA.peerId());
    courierB.setSignalService(&signalB);
    courierB.setAppSettings(&settingsB);
    courierB.setTransferRepository(&transfersB);
    courierB.setLocalPeerId(settingsB.peerId());
    signalA.setCourierService(&courierA);
    signalB.setCourierService(&courierB);

    QString error;
    QVERIFY2(signalA.start(QHostAddress::LocalHost, error), qPrintable(error));
    QVERIFY2(signalB.start(QHostAddress::LocalHost, error), qPrintable(error));

    connect(&courierB,
            &FengSui::CourierService::transferRequested,
            &courierB,
            [&courierB](const FengSui::TransferTask& task) {
                courierB.acceptTransfer(task.transferId);
            });

    const QByteArray payload("hello fengsui file transfer");
    const QString sourcePath = tempA.filePath(QStringLiteral("source.txt"));
    QVERIFY(writeFile(sourcePath, payload));

    const QString transferId = courierA.sendFile(peerFor(settingsB, portB),
                                                 sourcePath);
    QVERIFY(!transferId.isEmpty());

    QTRY_VERIFY_WITH_TIMEOUT(
        taskHasStatus(&transfersA, transferId, FengSui::TransferStatus::Completed),
        5000);
    QTRY_VERIFY_WITH_TIMEOUT(
        taskHasStatus(&transfersB, transferId, FengSui::TransferStatus::Completed),
        5000);

    const std::optional<FengSui::TransferTask> receivedTask =
        transfersB.getTask(transferId);
    QVERIFY(receivedTask.has_value());
    QCOMPARE(receivedTask->fileName, QStringLiteral("source.txt"));
    QCOMPARE(receivedTask->fileSize, static_cast<qint64>(payload.size()));
    QCOMPARE(receivedTask->transferredBytes, static_cast<qint64>(payload.size()));
    QCOMPARE(readFile(receivedTask->filePath), payload);
}

void CourierServiceTest::rejectedTransferMarksBothSidesRejected()
{
    QTemporaryDir tempA;
    QTemporaryDir tempB;
    QVERIFY(tempA.isValid());
    QVERIFY(tempB.isValid());

    const quint16 portA = reserveLocalPort();
    const quint16 portB = reserveLocalPort();
    QVERIFY(portA != 0);
    QVERIFY(portB != 0);

    FengSui::Database databaseA(tempA.filePath(QStringLiteral("a.db")));
    FengSui::Database databaseB(tempB.filePath(QStringLiteral("b.db")));
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
    settingsB.setDownloadDir(tempB.filePath(QStringLiteral("downloads")));

    FengSui::TransferRepository transfersA(&databaseA);
    FengSui::TransferRepository transfersB(&databaseB);
    FengSui::SignalService signalA(&settingsA);
    FengSui::SignalService signalB(&settingsB);
    FengSui::CourierService courierA;
    FengSui::CourierService courierB;
    courierA.setSignalService(&signalA);
    courierA.setAppSettings(&settingsA);
    courierA.setTransferRepository(&transfersA);
    courierA.setLocalPeerId(settingsA.peerId());
    courierB.setSignalService(&signalB);
    courierB.setAppSettings(&settingsB);
    courierB.setTransferRepository(&transfersB);
    courierB.setLocalPeerId(settingsB.peerId());
    signalA.setCourierService(&courierA);
    signalB.setCourierService(&courierB);

    QString error;
    QVERIFY2(signalA.start(QHostAddress::LocalHost, error), qPrintable(error));
    QVERIFY2(signalB.start(QHostAddress::LocalHost, error), qPrintable(error));

    connect(&courierB,
            &FengSui::CourierService::transferRequested,
            &courierB,
            [&courierB](const FengSui::TransferTask& task) {
                courierB.rejectTransfer(task.transferId, QStringLiteral("不接收"));
            });

    const QString sourcePath = tempA.filePath(QStringLiteral("reject.txt"));
    QVERIFY(writeFile(sourcePath, QByteArray("reject me")));

    const QString transferId = courierA.sendFile(peerFor(settingsB, portB),
                                                 sourcePath);
    QVERIFY(!transferId.isEmpty());

    QTRY_VERIFY_WITH_TIMEOUT(
        taskHasStatus(&transfersA, transferId, FengSui::TransferStatus::Rejected),
        5000);
    QTRY_VERIFY_WITH_TIMEOUT(
        taskHasStatus(&transfersB, transferId, FengSui::TransferStatus::Rejected),
        5000);
}

void CourierServiceTest::invalidDownloadDirectoryFailsBothSides()
{
    QTemporaryDir tempA;
    QTemporaryDir tempB;
    QVERIFY(tempA.isValid());
    QVERIFY(tempB.isValid());

    const quint16 portA = reserveLocalPort();
    const quint16 portB = reserveLocalPort();
    QVERIFY(portA != 0);
    QVERIFY(portB != 0);

    FengSui::Database databaseA(tempA.filePath(QStringLiteral("a.db")));
    FengSui::Database databaseB(tempB.filePath(QStringLiteral("b.db")));
    QVERIFY(databaseA.initialize());
    QVERIFY(databaseB.initialize());

    const QString invalidDownloadDir = tempB.filePath(QStringLiteral("not_a_dir"));
    QVERIFY(writeFile(invalidDownloadDir, QByteArray("this is a file")));

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
    settingsB.setDownloadDir(invalidDownloadDir);

    FengSui::TransferRepository transfersA(&databaseA);
    FengSui::TransferRepository transfersB(&databaseB);
    FengSui::SignalService signalA(&settingsA);
    FengSui::SignalService signalB(&settingsB);
    FengSui::CourierService courierA;
    FengSui::CourierService courierB;
    courierA.setSignalService(&signalA);
    courierA.setAppSettings(&settingsA);
    courierA.setTransferRepository(&transfersA);
    courierA.setLocalPeerId(settingsA.peerId());
    courierB.setSignalService(&signalB);
    courierB.setAppSettings(&settingsB);
    courierB.setTransferRepository(&transfersB);
    courierB.setLocalPeerId(settingsB.peerId());
    signalA.setCourierService(&courierA);
    signalB.setCourierService(&courierB);

    QString error;
    QVERIFY2(signalA.start(QHostAddress::LocalHost, error), qPrintable(error));
    QVERIFY2(signalB.start(QHostAddress::LocalHost, error), qPrintable(error));

    connect(&courierB,
            &FengSui::CourierService::transferRequested,
            &courierB,
            [&courierB](const FengSui::TransferTask& task) {
                courierB.acceptTransfer(task.transferId);
            });

    const QString sourcePath = tempA.filePath(QStringLiteral("fail.txt"));
    QVERIFY(writeFile(sourcePath, QByteArray("cannot save")));

    const QString transferId = courierA.sendFile(peerFor(settingsB, portB),
                                                 sourcePath);
    QVERIFY(!transferId.isEmpty());

    QTRY_VERIFY_WITH_TIMEOUT(
        taskHasStatus(&transfersB, transferId, FengSui::TransferStatus::Failed),
        5000);
    QTRY_VERIFY_WITH_TIMEOUT(
        taskHasStatus(&transfersA, transferId, FengSui::TransferStatus::Failed),
        5000);
}

} // namespace

QTEST_MAIN(CourierServiceTest)

#include "tst_courier_service.moc"
