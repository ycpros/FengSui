// TcpServer.cpp
// TCP 监听器实现：端口绑定、连接接受、TcpConnection 创建。

#include "network/TcpServer.h"
#include "network/TcpConnection.h"

#include <QAbstractSocket>
#include <QDebug>
#include <QStringList>
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
    const QHostAddress effectiveBindAddress = bindAddress.isNull()
        ? QHostAddress::LocalHost
        : bindAddress;
    BindEndpoint endpoint;
    endpoint.ip = effectiveBindAddress;
    endpoint.port = port;
    endpoint.interfaceName = effectiveBindAddress.toString();
    return start(QList<BindEndpoint>{endpoint}, errorOut);
}

bool TcpServer::start(const QList<BindEndpoint>& endpoints, QString& errorOut)
{
    if (m_running) {
        return true;
    }
    if (endpoints.isEmpty()) {
        errorOut = QStringLiteral("No TCP bind endpoints");
        return false;
    }

    QStringList failures;
    for (const BindEndpoint& endpoint : endpoints) {
        auto* server = new QTcpServer(this);
        connect(server, &QTcpServer::newConnection, this, &TcpServer::onNewConnection);

        if (!server->listen(endpoint.ip, endpoint.port)) {
            const QString failure = QStringLiteral("%1:%2 %3")
                                        .arg(endpoint.ip.toString())
                                        .arg(endpoint.port)
                                        .arg(server->errorString());
            failures.append(failure);
            qWarning() << "Failed to start TCP server on" << failure;
            server->deleteLater();
            continue;
        }

        BindEndpoint bound = endpoint;
        bound.port = server->serverPort();
        m_servers.append(server);
        m_boundEndpoints.append(bound);
        qInfo() << "TCP server started on" << bound.ip.toString() << bound.port;
    }

    if (m_servers.isEmpty()) {
        errorOut = failures.join(QStringLiteral("; "));
        if (errorOut.isEmpty()) {
            errorOut = QStringLiteral("No TCP bind endpoints could be started");
        }
        return false;
    }

    if (!failures.isEmpty()) {
        qWarning() << "TCP server partial bind failures:" << failures;
    }

    m_port = m_boundEndpoints.first().port;
    m_running = true;
    return true;
}

void TcpServer::stop()
{
    if (m_servers.isEmpty()) {
        m_running = false;
        return;
    }

    for (QTcpServer* server : m_servers) {
        server->close();
        server->deleteLater();
    }
    m_servers.clear();
    m_boundEndpoints.clear();
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

QList<BindEndpoint> TcpServer::boundEndpoints() const
{
    return m_boundEndpoints;
}

void TcpServer::onNewConnection()
{
    auto* server = qobject_cast<QTcpServer*>(sender());
    if (!server) {
        return;
    }

    // accept 所有待处理连接
    while (server->hasPendingConnections()) {
        QTcpSocket* socket = server->nextPendingConnection();
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
