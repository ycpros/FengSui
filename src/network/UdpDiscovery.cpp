// UdpDiscovery.cpp
// UDP 发现实现：加入多播组，发送和接收 presence JSON 报文。

#include "network/UdpDiscovery.h"

#include <QAbstractSocket>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QUdpSocket>

namespace FengSui {

namespace {

// UDP 发现固定使用 8788 端口，对应 06_protocol.md。
constexpr quint16 kDiscoveryPort = 8788;

// 站点本地管理范围内的多播地址，避免依赖系统级 mDNS 服务。
const QHostAddress kMulticastAddress(QStringLiteral("239.255.100.100"));

} // namespace

UdpDiscovery::UdpDiscovery(QObject* parent)
    : QObject(parent)
{
}

UdpDiscovery::~UdpDiscovery()
{
    stop();
}

bool UdpDiscovery::start(QString& errorOut)
{
    if (m_running) {
        return true;
    }

    m_socket = new QUdpSocket(this);
    connect(m_socket, &QUdpSocket::readyRead, this, &UdpDiscovery::onReadyRead);

    // 允许同一台机器运行多个实例，便于本地双实例调试发现协议。
    const bool bound = m_socket->bind(QHostAddress::AnyIPv4,
                                      kDiscoveryPort,
                                      QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    if (!bound) {
        errorOut = m_socket->errorString();
        qWarning() << "Failed to bind UDP discovery socket:" << errorOut;
        stop();
        return false;
    }

    // TTL=2 限制报文只在近邻网段传播，符合 MVP 的同网段边界。
    m_socket->setSocketOption(QAbstractSocket::MulticastTtlOption, 2);

    // 开启回环便于单机调试；BeaconService 会忽略本机 peer_id。
    m_socket->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 1);

    // 加入多播组后才能接收同组其他客户端的发现报文。
    if (!m_socket->joinMulticastGroup(kMulticastAddress)) {
        errorOut = m_socket->errorString();
        qWarning() << "Failed to join UDP discovery multicast group:" << errorOut;
        stop();
        return false;
    }

    m_running = true;
    qInfo() << "UDP discovery started on" << kMulticastAddress.toString() << kDiscoveryPort;
    return true;
}

void UdpDiscovery::stop()
{
    if (!m_socket) {
        m_running = false;
        return;
    }

    // 先退出多播组再关闭 socket，减少系统保留组成员状态的概率。
    if (m_running) {
        m_socket->leaveMulticastGroup(kMulticastAddress);
    }

    m_socket->close();
    m_socket->deleteLater();
    m_socket = nullptr;
    m_running = false;
}

bool UdpDiscovery::isRunning() const
{
    return m_running;
}

bool UdpDiscovery::sendHello(const QJsonObject& payload, QString& errorOut)
{
    QJsonObject message = payload;
    message.insert(QStringLiteral("type"), QStringLiteral("presence.hello"));
    return sendMessage(message, errorOut);
}

bool UdpDiscovery::sendHeartbeat(const QString& peerId, QString& errorOut)
{
    if (peerId.trimmed().isEmpty()) {
        errorOut = QStringLiteral("peer_id is empty");
        return false;
    }

    QJsonObject message;
    message.insert(QStringLiteral("type"), QStringLiteral("presence.heartbeat"));
    message.insert(QStringLiteral("peer_id"), peerId);
    return sendMessage(message, errorOut);
}

bool UdpDiscovery::sendGoodbye(const QString& peerId, QString& errorOut)
{
    if (peerId.trimmed().isEmpty()) {
        errorOut = QStringLiteral("peer_id is empty");
        return false;
    }

    QJsonObject message;
    message.insert(QStringLiteral("type"), QStringLiteral("presence.goodbye"));
    message.insert(QStringLiteral("peer_id"), peerId);
    return sendMessage(message, errorOut);
}

void UdpDiscovery::onReadyRead()
{
    while (m_socket && m_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        // 按 UDP 报文真实大小读取，避免截断 JSON。
        datagram.resize(static_cast<int>(m_socket->pendingDatagramSize()));

        QHostAddress senderAddress;
        quint16 senderPort = 0;
        const qint64 bytesRead = m_socket->readDatagram(datagram.data(),
                                                        datagram.size(),
                                                        &senderAddress,
                                                        &senderPort);
        if (bytesRead <= 0) {
            const QString error = m_socket->errorString();
            qWarning() << "Failed to read UDP discovery datagram:" << error;
            emit errorOccurred(error);
            continue;
        }

        // UDP 层只接受 JSON 对象，数组或纯文本都交给上层错误信号处理。
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(datagram, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            const QString error = QStringLiteral("Invalid UDP discovery JSON: %1")
                                      .arg(parseError.errorString());
            qWarning() << error;
            emit errorOccurred(error);
            continue;
        }

        emit datagramReceived(document.object(), senderAddress, senderPort);
    }
}

bool UdpDiscovery::sendMessage(const QJsonObject& message, QString& errorOut)
{
    if (!m_socket || !m_running) {
        errorOut = QStringLiteral("UDP discovery socket is not running");
        return false;
    }

    // 使用紧凑 JSON，降低广播报文体积并保持协议可读。
    const QByteArray payload = QJsonDocument(message).toJson(QJsonDocument::Compact);
    const qint64 bytesWritten = m_socket->writeDatagram(payload, kMulticastAddress, kDiscoveryPort);
    if (bytesWritten != payload.size()) {
        errorOut = m_socket->errorString();
        if (errorOut.isEmpty()) {
            // Qt 可能只返回短写但没有 socket 错误，保留明确失败原因。
            errorOut = QStringLiteral("Failed to write full UDP discovery datagram");
        }
        qWarning() << "Failed to send UDP discovery datagram:" << errorOut;
        return false;
    }

    return true;
}

} // namespace FengSui
