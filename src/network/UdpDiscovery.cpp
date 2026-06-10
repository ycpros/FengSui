// UdpDiscovery.cpp
// UDP 发现实现：监听 presence JSON 报文，并按授权接口发送 directed broadcast。

#include "network/UdpDiscovery.h"

#include <QAbstractSocket>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QtGlobal>
#include <QUdpSocket>

namespace FengSui {

namespace {

// UDP 发现固定使用 8788 端口，对应 06_protocol.md。
constexpr quint16 kDiscoveryPort = 8788;

QHostAddress broadcastAddress(const DiscoveryEndpoint& endpoint)
{
    if (endpoint.ip.protocol() != QAbstractSocket::IPv4Protocol) {
        return QHostAddress();
    }

    const int prefix = qBound(0, endpoint.prefixLength, 32);
    const quint32 mask = (prefix == 0) ? 0 : (0xFFFFFFFFu << (32 - prefix));
    const quint32 ip = endpoint.ip.toIPv4Address();
    return QHostAddress((ip & mask) | ~mask);
}

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
    return start({}, errorOut);
}

bool UdpDiscovery::start(const QList<DiscoveryEndpoint>& endpoints, QString& errorOut)
{
    if (m_running) {
        return true;
    }
    if (endpoints.isEmpty()) {
        errorOut = QStringLiteral("No authorized discovery interfaces");
        return false;
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

    m_endpoints = endpoints;
    m_running = true;
    qInfo() << "UDP discovery started on" << kDiscoveryPort
            << "authorized interfaces:" << m_endpoints.size();
    return true;
}

void UdpDiscovery::stop()
{
    if (!m_socket) {
        m_running = false;
        return;
    }

    m_socket->close();
    m_socket->deleteLater();
    m_socket = nullptr;
    m_endpoints.clear();
    m_running = false;
}

bool UdpDiscovery::isRunning() const
{
    return m_running;
}

QList<DiscoveryEndpoint> UdpDiscovery::discoveryEndpoints() const
{
    return m_endpoints;
}

bool UdpDiscovery::sendHello(const QJsonObject& payload, QString& errorOut)
{
    QJsonObject message = payload;
    message.insert(QStringLiteral("type"), QStringLiteral("presence.hello"));
    return sendMessage(message, errorOut);
}

bool UdpDiscovery::sendHello(const QJsonObject& payload,
                             const DiscoveryEndpoint& endpoint,
                             QString& errorOut)
{
    QJsonObject message = payload;
    message.insert(QStringLiteral("type"), QStringLiteral("presence.hello"));
    return sendMessageToEndpoint(message, endpoint, errorOut);
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
    if (m_endpoints.isEmpty()) {
        errorOut = QStringLiteral("No authorized discovery interfaces");
        return false;
    }

    bool anySent = false;
    QStringList errors;
    for (const DiscoveryEndpoint& endpoint : m_endpoints) {
        QString endpointError;
        if (sendMessageToEndpoint(message, endpoint, endpointError)) {
            anySent = true;
        } else {
            errors.append(QStringLiteral("%1 %2: %3")
                              .arg(endpoint.interfaceName,
                                   endpoint.ip.toString(),
                                   endpointError));
        }
    }

    if (!anySent) {
        errorOut = errors.join(QStringLiteral("; "));
        return false;
    }

    if (!errors.isEmpty()) {
        qWarning() << "UDP discovery partial send failure:" << errors;
    }
    return true;
}

bool UdpDiscovery::sendMessageToEndpoint(const QJsonObject& message,
                                         const DiscoveryEndpoint& endpoint,
                                         QString& errorOut)
{
    if (!m_socket || !m_running) {
        errorOut = QStringLiteral("UDP discovery socket is not running");
        return false;
    }

    const QHostAddress target = broadcastAddress(endpoint);
    if (target.isNull()) {
        errorOut = QStringLiteral("Invalid discovery endpoint address");
        return false;
    }

    // 使用紧凑 JSON，降低广播报文体积并保持协议可读。
    const QByteArray payload = QJsonDocument(message).toJson(QJsonDocument::Compact);
    const qint64 bytesWritten = m_socket->writeDatagram(payload, target, kDiscoveryPort);
    if (bytesWritten != payload.size()) {
        errorOut = m_socket->errorString();
        if (errorOut.isEmpty()) {
            // Qt 可能只返回短写但没有 socket 错误，保留明确失败原因。
            errorOut = QStringLiteral("Failed to write full UDP discovery datagram");
        }
        qWarning() << "Failed to send UDP discovery datagram to"
                   << target.toString() << ":" << errorOut;
        return false;
    }

    return true;
}

} // namespace FengSui
