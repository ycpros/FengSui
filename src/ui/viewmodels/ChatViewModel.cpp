// ChatViewModel.cpp

#include "ui/viewmodels/ChatViewModel.h"

#include "core/SignalService.h"
#include "core/CourierService.h"
#include "core/BeaconService.h"

#include <QFileInfo>

namespace FengSui {

ChatViewModel::ChatViewModel(QObject* parent)
    : QObject(parent)
    , m_conversations(new ConversationListModel(this))
    , m_messages(new MessageListModel(this))
{
}

void ChatViewModel::bind(SignalService* signalService, CourierService* courier,
                         BeaconService* beacon, const QString& localPeerId)
{
    if (m_signal) {
        disconnect(m_signal, nullptr, this, nullptr);
    }
    if (m_beacon) {
        disconnect(m_beacon, nullptr, this, nullptr);
    }
    m_signal = signalService;
    m_courier = courier;
    m_beacon = beacon;
    m_localPeerId = localPeerId;
    m_messages->setLocalPeerId(localPeerId);

    if (m_signal) {
        connect(m_signal, &SignalService::messageStored,
                this, &ChatViewModel::onMessageStored);
        connect(m_signal, &SignalService::messageStatusChanged,
                this, &ChatViewModel::onMessageStatusChanged);
        connect(m_signal, &SignalService::conversationUpdated,
                this, &ChatViewModel::onConversationUpdated);
        connect(m_signal, &SignalService::peerConnected,
                this, &ChatViewModel::onPeerConnected);
        connect(m_signal, &SignalService::peerDisconnected,
                this, &ChatViewModel::onPeerDisconnected);
        reloadConversations();
    }
}

QString ChatViewModel::displayNameForPeer(const QString& peerId) const
{
    if (m_signal) {
        const auto p = m_signal->peerById(peerId);
        if (p && !p->displayName.isEmpty()) {
            return p->displayName;
        }
    }
    return peerId;
}

bool ChatViewModel::onlineForPeer(const QString& peerId) const
{
    if (m_beacon) {
        const QList<PeerInfo> peers = m_beacon->peers();
        for (const PeerInfo& p : peers) {
            if (p.peerId == peerId) {
                return p.online;
            }
        }
    }
    return false;
}

ConversationListModel::Row ChatViewModel::toConvRow(const Conversation& c) const
{
    ConversationListModel::Row row;
    row.conv = c;
    row.displayName = displayNameForPeer(c.peerId);
    row.online = onlineForPeer(c.peerId);
    return row;
}

void ChatViewModel::reloadConversations()
{
    if (!m_signal) {
        return;
    }
    QList<ConversationListModel::Row> rows;
    const QList<Conversation> convs = m_signal->conversations();
    for (const Conversation& c : convs) {
        rows.append(toConvRow(c));
    }
    m_conversations->resetRows(rows);
}

QString ChatViewModel::activePeerName() const
{
    if (m_activePeer.peerId.isEmpty()) {
        return QString();
    }
    return m_activePeer.displayName.isEmpty() ? m_activePeer.peerId : m_activePeer.displayName;
}

bool ChatViewModel::activePeerOnline() const
{
    return !m_activePeer.peerId.isEmpty() && onlineForPeer(m_activePeer.peerId);
}

void ChatViewModel::openConversation(const QString& peerId)
{
    if (!m_signal) {
        return;
    }
    // 解析 PeerInfo（优先在线设备，其次历史记录）
    std::optional<PeerInfo> peer;
    if (m_beacon) {
        for (const PeerInfo& p : m_beacon->peers()) {
            if (p.peerId == peerId) { peer = p; break; }
        }
    }
    if (!peer) {
        peer = m_signal->peerById(peerId);
    }
    if (!peer) {
        return;
    }
    m_activePeer = *peer;

    const Conversation conv = m_signal->openConversation(*peer);
    m_activeConversationId = conv.conversationId;

    // 加载历史消息
    const QList<Message> history = m_signal->messageHistory(conv.conversationId);
    m_messages->resetFrom(history);

    // 会话列表 upsert（幂等）+ 标记已读
    m_conversations->upsert(toConvRow(conv));
    markRead();

    emit activeConversationChanged();
    emit activePeerOnlineChanged();
}

void ChatViewModel::openConversationRow(int row)
{
    const QString peerId = m_conversations->peerIdAt(row);
    if (!peerId.isEmpty()) {
        openConversation(peerId);
    }
}

void ChatViewModel::sendText(const QString& content)
{
    if (!m_signal || m_activePeer.peerId.isEmpty()) {
        return;
    }
    const QString trimmed = content.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    m_signal->sendMessage(m_activePeer, trimmed);
    // 发送后 messageStored 信号会把气泡追加进来
}

void ChatViewModel::sendFiles(const QList<QUrl>& urls)
{
    if (!m_courier || m_activePeer.peerId.isEmpty()) {
        return;
    }
    for (const QUrl& url : urls) {
        const QString path = url.toLocalFile();
        if (path.isEmpty()) {
            continue;
        }
        QFileInfo fi(path);
        if (fi.isDir()) {
            m_courier->sendFolder(m_activePeer, path);
        } else if (fi.isFile()) {
            m_courier->sendFile(m_activePeer, path);
        }
    }
}

void ChatViewModel::markRead()
{
    if (m_signal && !m_activeConversationId.isEmpty()) {
        m_signal->markConversationRead(m_activeConversationId);
    }
}

void ChatViewModel::onMessageStored(const Message& msg)
{
    // 属于当前会话 → 追加/更新气泡（幂等）
    if (msg.conversationId == m_activeConversationId) {
        m_messages->append(msg);
        // 当前会话且窗口聚焦，收到即视为已读
        markRead();
    }
    // 会话列表条目刷新（摘要/未读/时间）——onConversationUpdated 也会触发，
    // 两者都走幂等 upsert，不会产生重复行或递归。
}

void ChatViewModel::onMessageStatusChanged(const QString& messageId, int status)
{
    m_messages->updateStatus(messageId, status);
}

void ChatViewModel::onConversationUpdated(const QString& conversationId)
{
    if (!m_signal) {
        return;
    }
    // 仅刷新该会话一行，避免整表重载引发的刷新风暴。
    const QList<Conversation> convs = m_signal->conversations();
    for (const Conversation& c : convs) {
        if (c.conversationId == conversationId) {
            m_conversations->upsert(toConvRow(c));
            break;
        }
    }
}

void ChatViewModel::onPeerConnected(const QString& peerId)
{
    m_conversations->setOnline(peerId, true);
    if (peerId == m_activePeer.peerId) {
        emit activePeerOnlineChanged();
    }
}

void ChatViewModel::onPeerDisconnected(const QString& peerId)
{
    m_conversations->setOnline(peerId, false);
    if (peerId == m_activePeer.peerId) {
        emit activePeerOnlineChanged();
    }
}

} // namespace FengSui
