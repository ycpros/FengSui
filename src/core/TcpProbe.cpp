// TcpProbe.cpp
// TCP 连通性探针实现：QEventLoop + QTimer 同步等待模式。

#include "core/TcpProbe.h"

#include <QEventLoop>
#include <QTcpSocket>
#include <QTimer>

namespace FengSui {

TcpProbe::TcpProbe(QObject* parent)
    : QObject(parent)
{
}

bool TcpProbe::probe(const QString& host, quint16 port, int timeoutMs, QString& errorOut)
{
    QTcpSocket socket;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    bool connected = false;

    QObject::connect(&socket, &QTcpSocket::connected, [&]() {
        connected = true;
        loop.quit();
    });

    QObject::connect(&socket, &QTcpSocket::errorOccurred, [&](QAbstractSocket::SocketError) {
        errorOut = socket.errorString();
        loop.quit();
    });

    QObject::connect(&timer, &QTimer::timeout, [&]() {
        errorOut = QString::fromUtf8("连接超时");
        loop.quit();
    });

    timer.start(timeoutMs);
    socket.connectToHost(host, port);
    loop.exec();

    if (connected) {
        socket.disconnectFromHost();
        // 等待断开完成，最多 1 秒
        if (socket.state() != QAbstractSocket::UnconnectedState) {
            socket.waitForDisconnected(1000);
        }
    } else {
        // 取消正在进行的连接尝试并等待
        socket.abort();
    }

    return connected;
}

} // namespace FengSui
