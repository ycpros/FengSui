// SignalService.cpp
// TCP 消息通道编排服务实现：管理连接生命周期、消息路由、ACK 自动回复。

#include "core/SignalService.h"

#include "app/AppSettings.h"
#include "core/CourierService.h"
#include "network/Protocol.h"
#include "network/TcpConnection.h"
#include "network/TcpServer.h"
#include "platform/InterfaceEnumerator.h"
#include "storage/ConversationRepository.h"
#include "storage/MessageRepository.h"

#include <QAbstractSocket>
#include <QDateTime>
#include <QDebug>
#include <QHostAddress>
#include <QJsonObject>
#include <QUuid>

namespace FengSui {

namespace {

QList<NetworkInterfaceInfo> orderedInterfaces()
{
    QList<NetworkInterfaceInfo> ordered = InterfaceEnumerator::candidates();
    const QList<NetworkInterfaceInfo> all = InterfaceEnumerator::enumerate();
    for (const NetworkInterfaceInfo& iface : all) {
        bool exists = false;
        for (const NetworkInterfaceInfo& current : ordered) {
            if (current.id == iface.id && current.ip == iface.ip) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            ordered.append(iface);
        }
    }
    return ordered;
}

} // namespace

SignalService::SignalService(AppSettings* settings,
                             NetworkPolicy* networkPolicy,
                             QObject* parent)
    : QObject(parent)
    , m_settings(settings)
    , m_networkPolicy(networkPolicy)
    , m_server(new TcpServer(this))
{
    // 入站连接到来时加入待识别列表
    connect(m_server, &TcpServer::newConnection,
            this, &SignalService::onNewIncomingConnection);

    connect(m_server, &TcpServer::errorOccurred, this,
            [this](const QString& errorMessage) {
                qWarning() << "TCP server error:" << errorMessage;
                emit errorOccurred(QStringLiteral("TCP server: %1").arg(errorMessage));
            });
}

SignalService::~SignalService()
{
    stop();
}

bool SignalService::start(QString& errorOut)
{
    if (m_running) {
        return true;
    }

    if (!m_settings) {
        errorOut = QStringLiteral("AppSettings is not available");
        return false;
    }

    m_localPeerId = m_settings->peerId().trimmed();
    if (m_localPeerId.isEmpty()) {
        errorOut = QStringLiteral("Local peer_id is empty");
        return false;
    }

    const QList<BindEndpoint> endpoints = buildBindEndpoints();
    if (endpoints.isEmpty()) {
        errorOut = QStringLiteral("No authorized TCP bind endpoints");
        return false;
    }

    if (!m_server->start(endpoints, errorOut)) {
        return false;
    }

    m_running = true;
    qInfo() << "Signal service started, peer:" << m_localPeerId
            << "bound endpoints:" << m_server->boundEndpoints().size();
    return true;
}

bool SignalService::start(const QHostAddress& bindAddress, QString& errorOut)
{
    if (m_running) {
        return true;
    }

    if (!m_settings) {
        errorOut = QStringLiteral("AppSettings is not available");
        return false;
    }

    m_localPeerId = m_settings->peerId().trimmed();
    if (m_localPeerId.isEmpty()) {
        errorOut = QStringLiteral("Local peer_id is empty");
        return false;
    }

    const quint16 port = m_settings->listenPort();
    if (!m_server->start(bindAddress, port, errorOut)) {
        return false;
    }

    m_running = true;
    qInfo() << "Signal service started, peer:" << m_localPeerId
            << "address:" << bindAddress.toString()
            << "port:" << m_server->port();
    return true;
}

void SignalService::setNetworkPolicy(NetworkPolicy* networkPolicy)
{
    m_networkPolicy = networkPolicy;
}

void SignalService::stop()
{
    if (!m_running) {
        return;
    }

    // 关闭所有活跃连接
    const QList<TcpConnection*> connections = m_connections.values();
    for (TcpConnection* conn : connections) {
        conn->disconnect();
        conn->deleteLater();
    }

    // 关闭待识别的入站连接
    for (TcpConnection* conn : m_pendingConnections) {
        conn->disconnect();
        conn->deleteLater();
    }

    m_connections.clear();
    m_pendingConnections.clear();
    m_pendingMessages.clear();

    m_server->stop();
    m_running = false;

    qInfo() << "Signal service stopped";
}

bool SignalService::isRunning() const
{
    return m_running;
}

QString SignalService::sendMessage(const PeerInfo& peer, const QString& content)
{
    const QString messageId = generateMessageId();

    if (!m_running) {
        qWarning() << "Signal service is not running";
        return messageId;
    }

    // 构建消息 JSON
    const QJsonObject message = Protocol::buildTextMessage(
        messageId, m_localPeerId, peer.peerId, content);

    // ---- 持久化到 SQLite（发送前保存 Sending 状态） ----
    if (m_messageRepo && m_conversationRepo) {
        // 确保对等体信息已录入 peers 表
        m_conversationRepo->ensurePeer(peer);

        // 创建或获取会话
        Conversation conv = m_conversationRepo->createOrGetConversation(peer.peerId);
        const QString convId = conv.conversationId;

        // 构造消息模型
        Message msg;
        msg.messageId = messageId;
        msg.conversationId = convId;
        msg.senderId = m_localPeerId;
        msg.type = MessageType::Text;
        msg.content = content;
        msg.createdAt = QDateTime::currentDateTimeUtc();
        msg.status = MessageStatus::Sending;

        if (m_messageRepo->saveMessage(msg)) {
            emit messageStored(msg);

            // 更新会话最后消息摘要
            QString summary = content.left(50);
            m_conversationRepo->updateLastMessage(convId, summary, msg.createdAt);
            emit conversationUpdated(convId);
        }
    }

    if (peer.ip.trimmed().isEmpty() || peer.port == 0) {
        qWarning() << "Cannot send message: peer endpoint is incomplete"
                   << peer.peerId << peer.ip << peer.port;
        if (m_messageRepo) {
            m_messageRepo->updateMessageStatus(messageId, MessageStatus::Failed);
            emit messageStatusChanged(messageId,
                                      static_cast<int>(MessageStatus::Failed));
        }
        return messageId;
    }

    // 查找或创建连接
    TcpConnection* conn = ensureConnection(peer);
    bool sent = false;
    if (conn && conn->isConnected()) {
        // 已有活跃连接，直接发送
        sent = conn->sendMessage(message);
    } else {
        // 首次建连或连接尚未完成时，必须把首条消息入队，等待 connected 后统一发送。
        m_pendingMessages[peer.peerId].append(message);
    }

    // ---- 发送结果更新状态 ----
    if (m_messageRepo) {
        if (sent) {
            m_messageRepo->updateMessageStatus(messageId, MessageStatus::Sent);
            emit messageStatusChanged(messageId,
                                      static_cast<int>(MessageStatus::Sent));
        } else if (conn && conn->isConnected() && !sent) {
            // 连接存在但发送失败
            m_messageRepo->updateMessageStatus(messageId, MessageStatus::Failed);
            emit messageStatusChanged(messageId,
                                      static_cast<int>(MessageStatus::Failed));
        }
    }

    if (!sent && conn && conn->isConnected()) {
        qWarning() << "Failed to send message" << messageId << "to" << peer.peerId;
    }

    return messageId;
}

bool SignalService::isConnectedTo(const QString& peerId) const
{
    const TcpConnection* conn = findConnection(peerId);
    return conn && conn->isConnected();
}

TcpConnection* SignalService::sendJsonMessage(const PeerInfo& peer,
                                              const QJsonObject& message)
{
    if (!m_running) {
        qWarning() << "Signal service is not running, cannot send JSON message";
        return nullptr;
    }

    // 查找或创建连接
    TcpConnection* conn = ensureConnection(peer);

    if (conn && conn->isConnected()) {
        // 已有活跃连接，直接发送
        if (conn->sendMessage(message)) {
            return conn;
        } else {
            qWarning() << "Failed to send JSON message to" << peer.peerId;
            return conn;  // 连接仍在，但发送失败
        }
    } else if (!conn) {
        // ensureConnection 正在建立连接中，消息入队
        m_pendingMessages[peer.peerId].append(message);
        return nullptr;
    } else {
        // 连接存在但未 connected，加入待发送队列
        m_pendingMessages[peer.peerId].append(message);
        return conn;
    }
}

TcpConnection* SignalService::connectionTo(const QString& peerId) const
{
    return findConnection(peerId);
}

void SignalService::setCourierService(CourierService* service)
{
    m_courierService = service;
}

// ---- Private Slots ----

void SignalService::onNewIncomingConnection(TcpConnection* connection)
{
    if (!connection) {
        return;
    }

    // 监听该连接的消息、断开和错误信号
    connect(connection, &TcpConnection::messageReceived,
            this, &SignalService::onConnectionMessage);
    connect(connection, &TcpConnection::disconnected,
            this, &SignalService::onConnectionDisconnected);
    connect(connection, &TcpConnection::errorOccurred,
            this, &SignalService::onConnectionError);

    // 将二进制 chunk 数据转发给 CourierService
    if (m_courierService) {
        connect(connection, &TcpConnection::binaryChunkReceived,
                m_courierService, &CourierService::handleChunkData);
    }

    // 入站连接尚未识别对等体身份，加入待识别列表
    m_pendingConnections.append(connection);

    qInfo() << "New incoming connection, pending identification";
}

void SignalService::onConnectionMessage(const QJsonObject& message)
{
    auto* connection = qobject_cast<TcpConnection*>(sender());
    if (!connection) {
        qWarning() << "SignalService::onConnectionMessage called from unexpected sender";
        return;
    }

    // 如果该连接仍在待识别列表中，从首条消息的 from 字段识别对等体
    if (m_pendingConnections.contains(connection)) {
        identifyIncomingConnection(connection, message);
    }

    const QString type = Protocol::messageType(message);

    // 对文本消息自动回复 ACK 并持久化
    if (Protocol::isTextMessage(message)) {
        const QString messageId = message.value(QStringLiteral("message_id")).toString();
        const QString fromPeerId = message.value(QStringLiteral("from")).toString();
        const QString content = message.value(QStringLiteral("content")).toString();
        const QString createdAtStr = message.value(QStringLiteral("created_at")).toString();

        // ACK 自动回复
        if (!messageId.isEmpty()) {
            sendAck(messageId, QStringLiteral("delivered"));
        }

        // ---- 持久化收到的消息到 SQLite ----
        if (m_messageRepo && m_conversationRepo && !fromPeerId.isEmpty()) {
            Conversation conv = m_conversationRepo->createOrGetConversation(fromPeerId);
            const QString convId = conv.conversationId;

            Message msg;
            msg.messageId = messageId;
            msg.conversationId = convId;
            msg.senderId = fromPeerId;
            msg.type = MessageType::Text;
            msg.content = content;
            msg.createdAt = createdAtStr.isEmpty()
                ? QDateTime::currentDateTimeUtc()
                : QDateTime::fromString(createdAtStr, Qt::ISODate);
            msg.status = MessageStatus::Delivered;  // 已收到，视为已送达

            if (m_messageRepo->saveMessage(msg)) {
                emit messageStored(msg);

                // 更新会话最后消息和未读计数
                m_conversationRepo->updateLastMessage(
                    convId, content.left(50), msg.createdAt);
                m_conversationRepo->incrementUnreadCount(convId);
                emit conversationUpdated(convId);
            }
        }
    }

    // 收到 ACK 消息：更新对应消息的投递状态
    if (Protocol::isAck(message)) {
        const QString messageId = message.value(QStringLiteral("message_id")).toString();
        const QString status = message.value(QStringLiteral("status")).toString();
        qInfo() << "Received ACK for message" << messageId << "status:" << status;

        if (m_messageRepo && !messageId.isEmpty()) {
            MessageStatus newStatus = (status == QStringLiteral("delivered"))
                ? MessageStatus::Delivered : MessageStatus::Failed;
            m_messageRepo->updateMessageStatus(messageId, newStatus);
            emit messageStatusChanged(messageId, static_cast<int>(newStatus));
        }
    }

    // 将 transfer.* 族消息转发给 CourierService 处理
    if (m_courierService && Protocol::isTransferMessage(message)) {
        m_courierService->handleTransferMessage(connection, message);
    }

    // 向上层投递消息
    emit messageReceived(message);
}

void SignalService::onConnectionDisconnected()
{
    auto* connection = qobject_cast<TcpConnection*>(sender());
    if (!connection) {
        return;
    }

    const QString peerId = connection->peerId();

    if (!peerId.isEmpty()) {
        qInfo() << "Peer disconnected:" << peerId;
        failPendingMessages(peerId, QStringLiteral("连接已断开"));
        emit peerDisconnected(peerId);
        unregisterConnection(peerId);
    }

    // 从待识别列表中移除
    m_pendingConnections.removeAll(connection);

    connection->deleteLater();
}

void SignalService::onConnectionError(const QString& errorMessage)
{
    auto* connection = qobject_cast<TcpConnection*>(sender());
    const QString peerId = connection ? connection->peerId() : QString();

    qWarning() << "Connection error for peer" << peerId << ":" << errorMessage;
    if (!peerId.isEmpty()) {
        failPendingMessages(peerId, errorMessage);
    }
    emit errorOccurred(QStringLiteral("Peer %1: %2").arg(peerId, errorMessage));
}

void SignalService::onOutgoingConnected()
{
    auto* connection = qobject_cast<TcpConnection*>(sender());
    if (!connection) {
        return;
    }

    const QString peerId = connection->peerId();
    if (peerId.isEmpty()) {
        qWarning() << "Outgoing connection established but peerId is empty";
        return;
    }

    qInfo() << "Outgoing connection established to" << peerId;
    emit peerConnected(peerId);

    // 连接建立后刷新待发送消息队列
    flushPendingMessages(peerId);
}

// ---- Private Helpers ----

void SignalService::registerConnection(const QString& peerId, TcpConnection* connection)
{
    // 移除旧连接（如果存在）
    if (m_connections.contains(peerId)) {
        TcpConnection* old = m_connections.take(peerId);
        if (old && old != connection) {
            qInfo() << "Replacing existing connection for peer" << peerId;
            old->disconnect();
            old->deleteLater();
        }
    }

    m_connections.insert(peerId, connection);
}

void SignalService::unregisterConnection(const QString& peerId)
{
    m_connections.remove(peerId);
    // 注意：不在这里 deleteLater，由调用方管理生命周期
}

TcpConnection* SignalService::ensureConnection(const PeerInfo& peer)
{
    // 已有活跃连接，直接返回
    TcpConnection* existing = findConnection(peer.peerId);
    if (existing) {
        if (existing->isConnected()) {
            return existing;
        }
        // 连接存在但未连接（正在建立中），返回以让调用方将消息入队
        return existing;
    }

    // 创建出站连接
    auto* conn = new TcpConnection(this);
    conn->setPeerId(peer.peerId);

    // 连接信号
    connect(conn, &TcpConnection::connected,
            this, &SignalService::onOutgoingConnected);
    connect(conn, &TcpConnection::messageReceived,
            this, &SignalService::onConnectionMessage);
    connect(conn, &TcpConnection::disconnected,
            this, &SignalService::onConnectionDisconnected);
    connect(conn, &TcpConnection::errorOccurred,
            this, &SignalService::onConnectionError);

    // 将二进制 chunk 数据转发给 CourierService
    if (m_courierService) {
        connect(conn, &TcpConnection::binaryChunkReceived,
                m_courierService, &CourierService::handleChunkData);
    }

    // 注册到连接映射（占位，实际 connected 时才算建立）
    registerConnection(peer.peerId, conn);

    // 发起连接
    conn->connectToHost(peer.ip, peer.port);

    // 返回 nullptr 表示连接尚未就绪，消息由调用方加入待发送队列
    return nullptr;
}

TcpConnection* SignalService::findConnection(const QString& peerId) const
{
    return m_connections.value(peerId, nullptr);
}

QString SignalService::generateMessageId() const
{
    return QStringLiteral("msg_%1").arg(
        QUuid::createUuid().toString(QUuid::Id128));
}

void SignalService::sendAck(const QString& messageId, const QString& status)
{
    // ACK 回复给发送方，但由于 ACK 不需要目标地址，
    // 我们通过当前活跃连接回复（接收消息的连接即发送方连接）
    const QJsonObject ack = Protocol::buildAck(messageId, status);

    auto* connection = qobject_cast<TcpConnection*>(sender());
    if (connection && connection->isConnected()) {
        if (!connection->sendMessage(ack)) {
            qWarning() << "Failed to send ACK for message" << messageId;
        }
    }
}

void SignalService::identifyIncomingConnection(TcpConnection* connection,
                                               const QJsonObject& firstMessage)
{
    const QString fromPeerId = firstMessage.value(QStringLiteral("from")).toString().trimmed();

    if (fromPeerId.isEmpty()) {
        qWarning() << "First message from incoming connection has no 'from' field,"
                   << "cannot identify peer";
        return;
    }

    // 从待识别列表移除
    m_pendingConnections.removeAll(connection);

    // 处理可能的重复连接
    if (!resolveDuplicateConnection(fromPeerId, connection)) {
        // 重复连接解决失败（本连接被判定关闭），不注册
        qInfo() << "Incoming connection closed due to duplicate resolution:"
                << fromPeerId;
        connection->disconnect();
        connection->deleteLater();
        return;
    }

    // 注册对等体身份
    connection->setPeerId(fromPeerId);
    registerConnection(fromPeerId, connection);

    qInfo() << "Incoming connection identified as peer:" << fromPeerId;
    emit peerConnected(fromPeerId);
    flushPendingMessages(fromPeerId);
}

bool SignalService::resolveDuplicateConnection(const QString& peerId,
                                               TcpConnection* newConnection)
{
    TcpConnection* existing = findConnection(peerId);
    if (!existing) {
        // 无重复连接
        return true;
    }

    // 双方同时发起连接：peerId 字典序较小者保留
    // 使用本地 peerId 与对方 peerId 比较
    if (m_localPeerId < peerId) {
        // 本地 peerId 较小，保留已存在的出站连接，关闭新入站连接
        qInfo() << "Duplicate connection resolved: keeping outgoing connection to"
                << peerId << "(local peerId is smaller)";
        return false;
    }

    // 对端 peerId 较小，关闭已有连接，使用新入站连接
    qInfo() << "Duplicate connection resolved: switching to incoming connection from"
            << peerId << "(remote peerId is smaller)";
    existing->disconnect();
    existing->deleteLater();
    m_connections.remove(peerId);

    return true;
}

void SignalService::flushPendingMessages(const QString& peerId)
{
    auto it = m_pendingMessages.find(peerId);
    if (it == m_pendingMessages.end() || it.value().isEmpty()) {
        return;
    }

    TcpConnection* conn = findConnection(peerId);
    if (!conn || !conn->isConnected()) {
        qWarning() << "Cannot flush pending messages: no active connection to" << peerId;
        return;
    }

    const QList<QJsonObject> messages = it.value();
    qInfo() << "Flushing" << messages.size() << "pending messages to" << peerId;

    for (const QJsonObject& msg : messages) {
        const QString msgId = msg.value(QStringLiteral("message_id")).toString();
        if (!conn->sendMessage(msg)) {
            qWarning() << "Failed to flush pending message to" << peerId;
            if (m_messageRepo && !msgId.isEmpty()) {
                m_messageRepo->updateMessageStatus(msgId, MessageStatus::Failed);
                emit messageStatusChanged(msgId,
                                          static_cast<int>(MessageStatus::Failed));
            }
        } else {
            // 更新已发送消息的状态
            if (m_messageRepo && !msgId.isEmpty()) {
                m_messageRepo->updateMessageStatus(msgId, MessageStatus::Sent);
                emit messageStatusChanged(msgId,
                                          static_cast<int>(MessageStatus::Sent));
            }
        }
    }

    m_pendingMessages.remove(peerId);
}

void SignalService::failPendingMessages(const QString& peerId,
                                        const QString& errorMessage)
{
    auto it = m_pendingMessages.find(peerId);
    if (it == m_pendingMessages.end() || it.value().isEmpty()) {
        return;
    }

    const QList<QJsonObject> messages = it.value();
    qWarning() << "Failing" << messages.size()
               << "pending messages to" << peerId << ":" << errorMessage;

    for (const QJsonObject& msg : messages) {
        const QString msgId = msg.value(QStringLiteral("message_id")).toString();
        if (m_messageRepo && !msgId.isEmpty()) {
            m_messageRepo->updateMessageStatus(msgId, MessageStatus::Failed);
            emit messageStatusChanged(msgId,
                                      static_cast<int>(MessageStatus::Failed));
        }
    }

    m_pendingMessages.remove(peerId);
}

QList<BindEndpoint> SignalService::buildBindEndpoints() const
{
    if (!m_networkPolicy || !m_settings) {
        return {};
    }

    QList<InterfaceAddress> addresses;
    const QList<NetworkInterfaceInfo> interfaces = orderedInterfaces();
    for (const NetworkInterfaceInfo& iface : interfaces) {
        InterfaceAddress address;
        address.interfaceId = iface.id;
        address.interfaceName = iface.name;
        address.ip = iface.ip;
        addresses.append(address);
    }

    return m_networkPolicy->getBindEndpoints(m_settings->listenPort(), addresses);
}

// ---- 存储层注入与查询代理 ----

void SignalService::setConversationRepository(ConversationRepository* repo)
{
    m_conversationRepo = repo;
}

void SignalService::setMessageRepository(MessageRepository* repo)
{
    m_messageRepo = repo;
}

ConversationRepository* SignalService::conversationRepository() const
{
    return m_conversationRepo;
}

Conversation SignalService::openConversation(const PeerInfo& peer)
{
    if (!m_conversationRepo || peer.peerId.trimmed().isEmpty()) {
        return {};
    }

    m_conversationRepo->ensurePeer(peer);
    return m_conversationRepo->createOrGetConversation(peer.peerId);
}

QList<Conversation> SignalService::conversations() const
{
    if (!m_conversationRepo) {
        return {};
    }
    return m_conversationRepo->getAllConversations();
}

std::optional<PeerInfo> SignalService::peerById(const QString& peerId) const
{
    if (!m_conversationRepo) {
        return std::nullopt;
    }
    return m_conversationRepo->getPeer(peerId);
}

std::optional<PeerInfo> SignalService::peerForConversation(
    const QString& conversationId) const
{
    if (!m_conversationRepo) {
        return std::nullopt;
    }
    return m_conversationRepo->getPeerForConversation(conversationId);
}

QList<Message> SignalService::messageHistory(const QString& conversationId,
                                             int limit,
                                             int offset) const
{
    if (!m_messageRepo) {
        return {};
    }
    return m_messageRepo->getMessages(conversationId, limit, offset);
}

void SignalService::markConversationRead(const QString& conversationId)
{
    if (m_conversationRepo) {
        m_conversationRepo->resetUnreadCount(conversationId);
    }
}

} // namespace FengSui
