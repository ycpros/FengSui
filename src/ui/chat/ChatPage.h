// ChatPage.h
// 聊天页面：左侧会话列表 + 右侧聊天面板（消息气泡 + 输入框）。
// 依赖 SignalService 获取消息和发送消息，不直接访问存储层或网络层。

#pragma once

#include "models/Conversation.h"
#include "models/Message.h"
#include "models/PeerInfo.h"

#include <QHash>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QStackedWidget>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

namespace FengSui {

class CourierService;
class SignalService;
struct TransferTask;

// 聊天页面：QSplitter 分左右两栏。
// 左侧：会话列表（QListWidget），显示所有会话及最后消息摘要。
// 右侧：聊天面板（QStackedWidget 切换空态/聊天视图），包含消息气泡和输入框。
class ChatPage : public QWidget {
    Q_OBJECT

public:
    explicit ChatPage(QWidget* parent = nullptr);

    // 依赖注入：设置信号服务（由 MainWindow 在构造时调用）。
    void setSignalService(SignalService* service);

    // 依赖注入：设置文件传输服务（由 MainWindow 在构造时调用）。
    void setCourierService(CourierService* service);

    // 依赖注入：设置本机 peer_id（用于判断消息是否为自己发送）。
    void setLocalPeerId(const QString& peerId);

    // 打开与指定对等体的会话（从 MainWindow 的 peerActivated 信号驱动）。
    void openConversation(const PeerInfo& peer);

    // 从数据库刷新会话列表（MainWindow 初始加载时调用）。
    void refreshConversationList();

private slots:
    // 点击发送按钮或按 Enter 键
    void onSendClicked();

    // SignalService 发来的消息已持久化
    void onMessageStored(const Message& message);

    // 消息状态变更（sending → sent → delivered → failed）
    void onMessageStatusChanged(const QString& messageId, int status);

    // 会话列表选中变化
    void onConversationSelected(QListWidgetItem* current, QListWidgetItem* previous);

    // 会话元信息变更（最后消息、未读计数）
    void onConversationUpdated(const QString& conversationId);

    // 收到新的文件传输请求。
    void onTransferRequested(const TransferTask& task);

private:
    // 构建界面布局
    void setupUi();

    // 连接 SignalService 信号
    void setupConnections();

    // 创建消息气泡控件
    QWidget* createMessageBubble(const Message& message);

    // 将消息气泡追加到聊天区域底部
    void appendMessage(const Message& message);

    // 清空聊天区域的所有消息气泡
    void clearMessageBubbles();

    // 更新已存在气泡的状态指示器文本
    void updateMessageStatusInBubble(const QString& messageId, MessageStatus status);

    // 为会话列表项创建自定义 widget
    QWidget* createConversationItemWidget(const Conversation& conv);

    // 滚动消息区域到底部
    void scrollToBottom();

    // 判断用户是否已滚动到接近底部（用于智能自动滚动）
    bool isNearBottom() const;

    // 处理拖入的本地文件 URL。
    bool handleDroppedUrls(const QList<QUrl>& urls);

    // 事件过滤器：处理输入框 Enter/Shift+Enter 键
    bool eventFilter(QObject* obj, QEvent* event) override;

    SignalService* m_signalService = nullptr;
    CourierService* m_courierService = nullptr;
    QString m_localPeerId;
    PeerInfo m_activePeer;
    QString m_activeConversationId;

    // ---- 布局组件 ----
    QSplitter*      m_splitter = nullptr;
    // 左侧会话列表
    QListWidget*    m_conversationList = nullptr;
    QLabel*         m_emptyConvLabel = nullptr;
    // 右侧聊天区
    QStackedWidget* m_chatStack = nullptr;
    QLabel*         m_emptyChatLabel = nullptr;
    QWidget*        m_chatPanel = nullptr;
    QLabel*         m_peerNameLabel = nullptr;
    QLabel*         m_peerStatusLabel = nullptr;
    QLabel*         m_offlineHintLabel = nullptr;
    QScrollArea*    m_messageScroll = nullptr;
    QWidget*        m_messageContainer = nullptr;
    QVBoxLayout*    m_messageLayout = nullptr;
    QLabel*         m_dropHintLabel = nullptr;
    QTextEdit*      m_inputEdit = nullptr;
    QPushButton*    m_sendBtn = nullptr;

    // ---- 运行时数据 ----
    // messageId → 气泡 time/status 标签映射（用于状态更新时快速定位）
    QHash<QString, QLabel*> m_bubbleStatusLabels;

    // 已加载的会话元信息缓存（conversationId → Conversation）
    QHash<QString, Conversation> m_conversationCache;
};

} // namespace FengSui
