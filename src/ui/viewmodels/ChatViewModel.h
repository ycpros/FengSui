// ChatViewModel.h
// 聊天页 ViewModel：会话列表 + 当前会话消息，连接 SignalService 消息信号，
// 处理发送文本与拖拽发送文件。按 id 幂等更新，避免重复气泡/刷新递归。

#pragma once

#include "models/PeerInfo.h"
#include "ui/viewmodels/ConversationListModel.h"
#include "ui/viewmodels/MessageListModel.h"

#include <QObject>
#include <QString>
#include <QUrl>
#include <QList>

namespace FengSui {

class SignalService;
class CourierService;
class BeaconService;

class ChatViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(ConversationListModel* conversations READ conversations CONSTANT)
    Q_PROPERTY(MessageListModel* messages READ messages CONSTANT)
    Q_PROPERTY(bool hasActiveConversation READ hasActiveConversation NOTIFY activeConversationChanged)
    Q_PROPERTY(QString activePeerName READ activePeerName NOTIFY activeConversationChanged)
    Q_PROPERTY(bool activePeerOnline READ activePeerOnline NOTIFY activePeerOnlineChanged)

public:
    explicit ChatViewModel(QObject* parent = nullptr);

    void bind(SignalService* signalService, CourierService* courier,
              BeaconService* beacon, const QString& localPeerId);

    ConversationListModel* conversations() const { return m_conversations; }
    MessageListModel* messages() const { return m_messages; }

    bool hasActiveConversation() const { return !m_activeConversationId.isEmpty(); }
    QString activePeerName() const;
    bool activePeerOnline() const;

    // 打开与某设备的会话（联系人页双击、或会话列表点击）
    Q_INVOKABLE void openConversation(const QString& peerId);
    // 按会话列表行打开
    Q_INVOKABLE void openConversationRow(int row);
    Q_INVOKABLE void sendText(const QString& content);
    Q_INVOKABLE void sendFiles(const QList<QUrl>& urls);
    Q_INVOKABLE void markRead();

signals:
    void activeConversationChanged();
    void activePeerOnlineChanged();

private slots:
    void onMessageStored(const Message& msg);
    void onMessageStatusChanged(const QString& messageId, int status);
    void onConversationUpdated(const QString& conversationId);
    void onPeerConnected(const QString& peerId);
    void onPeerDisconnected(const QString& peerId);

private:
    void reloadConversations();
    ConversationListModel::Row toConvRow(const Conversation& c) const;
    QString displayNameForPeer(const QString& peerId) const;
    bool onlineForPeer(const QString& peerId) const;

    SignalService*  m_signal = nullptr;
    CourierService* m_courier = nullptr;
    BeaconService*  m_beacon = nullptr;
    QString         m_localPeerId;

    ConversationListModel* m_conversations = nullptr;
    MessageListModel*      m_messages = nullptr;

    PeerInfo m_activePeer;
    QString  m_activeConversationId;
};

} // namespace FengSui
