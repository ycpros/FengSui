// TcpServer.cpp
// TCP 监听器实现：端口绑定、连接接受、TcpConnection 创建。

#include "network/TcpServer.h"
#include "network/TcpConnection.h"

#include <QAbstractSocket>
#include <QDebug>
#include <QTcpServer>
#include <QTcpSocket>

namespace FengSui {

TcpServer::TcpServer(QObject* parent)
    : QObject(parent)
{
}

TcpServer::~TcpServer()
{
    stop();
}

bool TcpServer::start(quint16 port, QString& errorOut)
{
    return start(QHostAddress::LocalHost, port, errorOut);
}

bool TcpServer::start(const QHostAddress& bindAddress, quint16 port, QString& errorOut)
{
    if (m_running) {
        return true;
    }

    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &TcpServer::onNewConnection);

    const QHostAddress effectiveBindAddress = bindAddress.isNull()
        ? QHostAddress::LocalHost
        : bindAddress;

    // ShareAddress + ReuseAddressHint 允许同一台机器运行多个实例。
    // 绑定地址由 SignalService 选择，避免默认监听 0.0.0.0。
    if (!m_server->listen(effectiveBindAddress, port)) {
        errorOut = m_server->errorString();
        qWarning() << "Failed to start TCP server on"
                   << effectiveBindAddress.toString()
                   << port
                   << ":"
                   << errorOut;
        m_server->deleteLater();
        m_server = nullptr;
        return false;
    }

    m_bindAddress = effectiveBindAddress;
    m_port = m_server->serverPort();
    m_running = true;

    qInfo() << "TCP server started on" << m_bindAddress.toString() << m_port;
    return true;
}

void TcpServer::stop()
{
    if (!m_server) {
        m_running = false;
        return;
    }

    m_server->close();
    m_server->deleteLater();
    m_server = nullptr;
    m_bindAddress = QHostAddress();
    m_running = false;
    m_port = 0;

    qInfo() << "TCP server stopped";
}

bool TcpServer::isRunning() const
{
    return m_running;
}

quint16 TcpServer::port() const
{
    return m_port;
}

void TcpServer::onNewConnection()
{
    if (!m_server) {
        return;
    }

    // accept 所有待处理连接
    while (m_server->hasPendingConnections()) {
        QTcpSocket* socket = m_server->nextPendingConnection();
        if (!socket) {
            continue;
        }

        // 为每个入站连接创建 TcpConnection 封装；TcpConnection 会接管 socket 生命周期。
        auto* connection = new TcpConnection(this);
        connection->setSocket(socket);

        qInfo() << "Accepted new TCP connection from"
                << socket->peerAddress().toString() << socket->peerPort();

        emit newConnection(connection);
    }
}

} // namespace FengSui
