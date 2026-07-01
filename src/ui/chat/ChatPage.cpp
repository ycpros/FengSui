// ChatPage.cpp
// 聊天页面实现：会话列表、消息气泡、输入发送。

#include "ui/chat/ChatPage.h"

#include "core/CourierService.h"
#include "core/SignalService.h"
#include "ui/UiStyle.h"

#include <QDateTime>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include <optional>

namespace FengSui {

// 会话列表项高度（像素）
static constexpr int kConversationItemHeight = 64;
// 消息气泡最大宽度（像素）
static constexpr int kBubbleMaxWidth = 400;
// 左侧会话列表初始宽度（像素）
static constexpr int kConvListWidth = 280;
// 自动滚动阈值：距底部多少像素以内视为"在底部"（像素）
static constexpr int kScrollNearBottomThreshold = 50;

ChatPage::ChatPage(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void ChatPage::setSignalService(SignalService* service)
{
    m_signalService = service;
    if (m_signalService) {
        setupConnections();
        // 初始加载会话列表
        refreshConversationList();
    }
}

void ChatPage::setCourierService(CourierService* service)
{
    if (m_courierService == service) {
        return;
    }
    if (m_courierService) {
        disconnect(m_courierService, nullptr, this, nullptr);
    }

    m_courierService = service;
    if (m_courierService) {
        connect(m_courierService,
                &CourierService::transferRequested,
                this,
                &ChatPage::onTransferRequested);
    }
}

void ChatPage::setLocalPeerId(const QString& peerId)
{
    m_localPeerId = peerId;
}

void ChatPage::openConversation(const PeerInfo& peer)
{
    if (peer.peerId.isEmpty()) {
        return;
    }

    m_activePeer = peer;

    // 切换到聊天面板（显示聊天视图）
    m_chatStack->setCurrentIndex(1);

    // 更新头部信息
    m_peerNameLabel->setText(peer.displayName.isEmpty() ? peer.peerId : peer.displayName);
    if (peer.online) {
        m_peerStatusLabel->setText(QStringLiteral("在线"));
        m_peerStatusLabel->setStyleSheet(UiStyle::pillStyle(QStringLiteral("#e7f5f4"),
                                                            QStringLiteral("#0f6f70")));
    } else {
        m_peerStatusLabel->setText(QStringLiteral("离线"));
        m_peerStatusLabel->setStyleSheet(UiStyle::pillStyle(QStringLiteral("#eef1f5"),
                                                            QStringLiteral("#667085")));
    }
    if (m_offlineHintLabel) {
        m_offlineHintLabel->setVisible(!peer.online);
    }

    // 获取或创建会话 ID
    m_activeConversationId.clear();
    if (m_signalService) {
        // UI 不直接访问 storage，由 SignalService 同步 peer 并创建会话。
        Conversation conv = m_signalService->openConversation(peer);
        m_activeConversationId = conv.conversationId;
    }

    // 清空现有气泡并加载历史消息
    clearMessageBubbles();

    if (m_signalService && !m_activeConversationId.isEmpty()) {
        QList<Message> history = m_signalService->messageHistory(m_activeConversationId);
        for (const Message& msg : history) {
            appendMessage(msg);
        }
        scrollToBottom();

        // 标记已读（未读计数归零）
        m_signalService->markConversationRead(m_activeConversationId);
    }

    // 刷新会话列表（更新选中状态和未读徽章）
    refreshConversationList();

    // 聚焦输入框
    m_inputEdit->setFocus();
}

void ChatPage::refreshConversationList()
{
    if (!m_signalService) {
        return;
    }

    QList<Conversation> conversations = m_signalService->conversations();

    // 缓存会话信息
    m_conversationCache.clear();
    for (const auto& conv : conversations) {
        m_conversationCache.insert(conv.conversationId, conv);
    }

    // 保存当前选中的会话 ID；没有当前项时优先恢复活跃会话。
    QString selectedConvId;
    if (m_conversationList->currentItem()) {
        selectedConvId = m_conversationList->currentItem()->data(Qt::UserRole).toString();
    } else {
        selectedConvId = m_activeConversationId;
    }

    // 重建会话列表
    QSignalBlocker blocker(m_conversationList);
    m_conversationList->clear();
    for (const auto& conv : conversations) {
        auto* item = new QListWidgetItem();
        item->setData(Qt::UserRole, conv.conversationId);
        item->setSizeHint(QSize(0, kConversationItemHeight));
        m_conversationList->addItem(item);
        m_conversationList->setItemWidget(item, createConversationItemWidget(conv));

        // 恢复选中状态
        if (conv.conversationId == selectedConvId) {
            m_conversationList->setCurrentItem(item);
        }
    }

    // 更新空态显示
    bool empty = conversations.isEmpty();
    m_emptyConvLabel->setVisible(empty);
    m_conversationList->setVisible(!empty);
}

// ---- Private Slots ----

void ChatPage::onSendClicked()
{
    QString content = m_inputEdit->toPlainText().trimmed();
    if (content.isEmpty() || !m_signalService || m_activePeer.peerId.isEmpty()) {
        return;
    }

    // 发送消息（SignalService 内部会持久化并发出 messageStored 信号）
    m_signalService->sendMessage(m_activePeer, content);

    // 清空输入框
    m_inputEdit->clear();
    m_inputEdit->setFocus();
}

void ChatPage::onMessageStored(const Message& message)
{
    // 仅当消息属于当前活跃会话时才追加气泡
    if (message.conversationId == m_activeConversationId) {
        appendMessage(message);
    }
    // 无论哪个会话，都刷新会话列表（更新最后消息摘要）
    refreshConversationList();
}

void ChatPage::onMessageStatusChanged(const QString& messageId, int status)
{
    updateMessageStatusInBubble(messageId, static_cast<MessageStatus>(status));
}

void ChatPage::onConversationSelected(QListWidgetItem* current, QListWidgetItem* previous)
{
    Q_UNUSED(previous);
    if (!current) {
        return;
    }

    const QString convId = current->data(Qt::UserRole).toString();
    if (convId.isEmpty() || !m_signalService) {
        return;
    }
    if (convId == m_activeConversationId) {
        return;
    }

    // 从缓存中获取会话信息
    auto it = m_conversationCache.find(convId);
    if (it == m_conversationCache.end()) {
        return;
    }

    // 从 core 层恢复完整 PeerInfo，保证历史会话再次发送时仍有 IP/端口路由。
    std::optional<PeerInfo> peer = m_signalService->peerForConversation(convId);
    if (!peer.has_value()) {
        peer = m_signalService->peerById(it->peerId);
    }
    if (!peer.has_value()) {
        PeerInfo fallbackPeer;
        fallbackPeer.peerId = it->peerId;
        fallbackPeer.displayName = it->peerId;
        peer = fallbackPeer;
    }

    openConversation(peer.value());
}

void ChatPage::onConversationUpdated(const QString& conversationId)
{
    Q_UNUSED(conversationId);
    // 简单实现：刷新整个会话列表
    // 后续可优化为只更新对应项
    refreshConversationList();
}

void ChatPage::onTransferRequested(const TransferTask& task)
{
    if (!m_courierService) {
        return;
    }

    const qint64 kib = qMax<qint64>(1, (task.fileSize + 1023) / 1024);
    const QString message = QStringLiteral("%1 请求发送文件：\n%2\n大小：%3 KB")
                                .arg(task.peerId, task.fileName)
                                .arg(kib);
    const QMessageBox::StandardButton button =
        QMessageBox::question(this,
                              QStringLiteral("接收文件"),
                              message,
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::Yes);
    if (button == QMessageBox::Yes) {
        m_courierService->acceptTransfer(task.transferId);
    } else {
        m_courierService->rejectTransfer(task.transferId,
                                         QStringLiteral("用户拒绝"));
    }
}

// ---- UI Setup ----

void ChatPage::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // QSplitter：左侧会话列表 + 右侧聊天面板
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setHandleWidth(1);

    // ---- 左侧：会话列表 ----
    auto* leftPanel = new QWidget(m_splitter);
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    // 标题
    auto* titleLabel = new QLabel(QStringLiteral("消息"), leftPanel);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: 700; padding: 16px; color: #101828;");

    // 会话列表
    m_conversationList = new QListWidget(leftPanel);
    m_conversationList->setFocusPolicy(Qt::NoFocus);
    m_conversationList->setObjectName(QStringLiteral("conversationList"));
    m_conversationList->setStyleSheet(
        "QListWidget {"
        "  border: none; border-right: 1px solid #d8dee8;"
        "  background-color: #ffffff;"
        "}"
        "QListWidget::item {"
        "  border: none; border-bottom: 1px solid #eef1f5;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #e7f5f4;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: #f5f9fa;"
        "}"
    );

    // 空态提示
    m_emptyConvLabel = new QLabel(
        QStringLiteral("暂无会话\n点击联系人中的在线设备开始聊天"),
        leftPanel);
    m_emptyConvLabel->setAlignment(Qt::AlignCenter);
    m_emptyConvLabel->setWordWrap(true);
    m_emptyConvLabel->setStyleSheet("color: #667085; font-size: 13px; padding: 40px;");
    m_emptyConvLabel->setVisible(true);
    m_conversationList->setVisible(false);

    leftLayout->addWidget(titleLabel);
    leftLayout->addWidget(m_emptyConvLabel);
    leftLayout->addWidget(m_conversationList);

    // ---- 右侧：聊天面板 ----
    m_chatStack = new QStackedWidget(m_splitter);

    // 页面 0：空态
    m_emptyChatLabel = new QLabel(
        QStringLiteral("选择一个会话开始聊天"),
        m_chatStack);
    m_emptyChatLabel->setAlignment(Qt::AlignCenter);
    m_emptyChatLabel->setStyleSheet("color: #667085; font-size: 14px; background: #f8fafc;");
    m_chatStack->addWidget(m_emptyChatLabel);  // index 0

    // 页面 1：聊天视图
    m_chatPanel = new QWidget(m_chatStack);
    m_chatPanel->setAcceptDrops(true);
    m_chatPanel->installEventFilter(this);
    auto* chatLayout = new QVBoxLayout(m_chatPanel);
    chatLayout->setContentsMargins(0, 0, 0, 0);
    chatLayout->setSpacing(0);

    // 头部栏：对方昵称 + 在线状态
    auto* headerBar = new QWidget(m_chatPanel);
    headerBar->setFixedHeight(56);
    headerBar->setStyleSheet("background-color: #ffffff; border-bottom: 1px solid #d8dee8;");
    auto* headerLayout = new QHBoxLayout(headerBar);
    headerLayout->setContentsMargins(16, 0, 16, 0);

    m_peerNameLabel = new QLabel(headerBar);
    m_peerNameLabel->setStyleSheet("font-size: 16px; font-weight: 700; border: none; color: #101828;");
    m_peerStatusLabel = new QLabel(headerBar);
    m_peerStatusLabel->setAlignment(Qt::AlignCenter);
    m_peerStatusLabel->setStyleSheet(UiStyle::pillStyle(QStringLiteral("#e7f5f4"),
                                                        QStringLiteral("#0f6f70")));

    headerLayout->addWidget(m_peerNameLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_peerStatusLabel);

    // 消息区域（QScrollArea）
    m_messageScroll = new QScrollArea(m_chatPanel);
    m_messageScroll->setAcceptDrops(true);
    m_messageScroll->setWidgetResizable(true);
    m_messageScroll->setFrameShape(QFrame::NoFrame);
    m_messageScroll->setStyleSheet("QScrollArea { background-color: #f8fafc; border: none; }");
    m_messageScroll->viewport()->setAcceptDrops(true);
    m_messageScroll->viewport()->installEventFilter(this);

    m_messageContainer = new QWidget(m_messageScroll);
    m_messageContainer->setAcceptDrops(true);
    m_messageContainer->installEventFilter(this);
    m_messageLayout = new QVBoxLayout(m_messageContainer);
    m_messageLayout->setContentsMargins(8, 8, 8, 8);
    m_messageLayout->setSpacing(6);
    m_messageLayout->addStretch();  // 底部弹簧，将气泡推到顶部

    m_messageScroll->setWidget(m_messageContainer);

    m_offlineHintLabel = new QLabel(QStringLiteral("对方当前离线，消息可能无法送达"), m_chatPanel);
    m_offlineHintLabel->setObjectName(QStringLiteral("chatOfflineHintLabel"));
    m_offlineHintLabel->setStyleSheet(UiStyle::pillStyle(QStringLiteral("#fff4d6"),
                                                         QStringLiteral("#8a5a00")));
    m_offlineHintLabel->setVisible(false);

    m_dropHintLabel = new QLabel(QStringLiteral("拖入单个本地文件即可发送"), m_chatPanel);
    m_dropHintLabel->setObjectName(QStringLiteral("chatDropHintLabel"));
    m_dropHintLabel->setAlignment(Qt::AlignCenter);
    m_dropHintLabel->setFixedHeight(30);
    m_dropHintLabel->setStyleSheet(
        "background: #eef4ff; color: #175cd3; border-top: 1px solid #d8dee8;"
        "font-size: 12px;");

    // 输入栏
    auto* inputBar = new QWidget(m_chatPanel);
    inputBar->setFixedHeight(64);
    inputBar->setStyleSheet("background-color: #ffffff; border-top: 1px solid #d8dee8;");
    auto* inputLayout = new QHBoxLayout(inputBar);
    inputLayout->setContentsMargins(12, 8, 12, 8);

    m_inputEdit = new QTextEdit(inputBar);
    m_inputEdit->setAcceptDrops(true);
    m_inputEdit->setPlaceholderText(
        QStringLiteral("输入消息..."));
    m_inputEdit->setMinimumHeight(36);
    m_inputEdit->setMaximumHeight(100);
    m_inputEdit->setStyleSheet(
        "border: 1px solid #d8dee8; border-radius: 6px; padding: 6px; font-size: 14px;"
        "background: #ffffff;");
    // 安装事件过滤器以处理 Enter 键
    m_inputEdit->installEventFilter(this);

    m_sendBtn = new QPushButton(
        QStringLiteral("发送"), inputBar);
    m_sendBtn->setFixedSize(72, 36);
    m_sendBtn->setEnabled(false);
    m_sendBtn->setStyleSheet(UiStyle::primaryButtonStyle());

    inputLayout->addWidget(m_inputEdit, 1);
    inputLayout->addWidget(m_sendBtn);

    chatLayout->addWidget(headerBar);
    chatLayout->addWidget(m_offlineHintLabel);
    chatLayout->addWidget(m_messageScroll, 1);
    chatLayout->addWidget(m_dropHintLabel);
    chatLayout->addWidget(inputBar);

    m_chatStack->addWidget(m_chatPanel);  // index 1
    m_chatStack->setCurrentIndex(0);       // 默认显示空态

    // 设置 QSplitter 初始比例
    m_splitter->addWidget(leftPanel);
    m_splitter->addWidget(m_chatStack);
    m_splitter->setSizes({kConvListWidth, 600});
    m_splitter->setStretchFactor(0, 0);  // 左侧固定
    m_splitter->setStretchFactor(1, 1);  // 右侧拉伸

    mainLayout->addWidget(m_splitter);

    // ---- 信号连接 ----
    // 发送按钮：点击发送
    connect(m_sendBtn, &QPushButton::clicked,
            this, &ChatPage::onSendClicked);

    // 输入框文本变化：控制发送按钮启用状态
    connect(m_inputEdit, &QTextEdit::textChanged, this, [this]() {
        m_sendBtn->setEnabled(!m_inputEdit->toPlainText().trimmed().isEmpty());
    });

    // 会话列表选中变化：打开对应会话
    connect(m_conversationList, &QListWidget::currentItemChanged,
            this, &ChatPage::onConversationSelected);
}

void ChatPage::setupConnections()
{
    if (!m_signalService) {
        return;
    }

    // 消息持久化后通知界面更新
    connect(m_signalService, &SignalService::messageStored,
            this, &ChatPage::onMessageStored);

    // 消息状态变更通知（sending → sent → delivered → failed）
    connect(m_signalService, &SignalService::messageStatusChanged,
            this, &ChatPage::onMessageStatusChanged);

    // 会话元信息变更通知（最后消息、未读计数）
    connect(m_signalService, &SignalService::conversationUpdated,
            this, &ChatPage::onConversationUpdated);
}

// ---- Message Bubble ----

QWidget* ChatPage::createMessageBubble(const Message& message)
{
    bool isMine = (message.senderId == m_localPeerId);

    auto* bubble = new QWidget(m_messageContainer);
    auto* outerLayout = new QHBoxLayout(bubble);
    outerLayout->setContentsMargins(8, 3, 8, 3);

    // 内容块：消息文字 + 时间/状态
    auto* innerWidget = new QWidget(bubble);
    auto* innerLayout = new QVBoxLayout(innerWidget);
    innerLayout->setContentsMargins(0, 0, 0, 0);
    innerLayout->setSpacing(2);

    // 消息内容标签
    auto* contentLabel = new QLabel(message.content, innerWidget);
    contentLabel->setWordWrap(true);
    contentLabel->setMaximumWidth(kBubbleMaxWidth);
    contentLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    contentLabel->setTextFormat(Qt::PlainText);  // 防止 HTML 注入

    // 时间和状态标签
    QString metaText = message.createdAt.toLocalTime().toString("HH:mm");
    if (isMine) {
        // 状态指示器
        switch (message.status) {
        case MessageStatus::Sending:   metaText += QStringLiteral(" ⏳"); break;
        case MessageStatus::Sent:      metaText += QStringLiteral(" ✓"); break;
        case MessageStatus::Delivered: metaText += QStringLiteral(" ✓✓"); break;
        case MessageStatus::Failed:    metaText += QStringLiteral(" ✗"); break;
        }
    }
    auto* metaLabel = new QLabel(metaText, innerWidget);
    metaLabel->setStyleSheet("font-size: 10px; color: #999; background: transparent;");
    metaLabel->setObjectName("meta_" + message.messageId);

    // 注册气泡状态标签以便后续更新
    m_bubbleStatusLabels.insert(message.messageId, metaLabel);

    innerLayout->addWidget(contentLabel);
    innerLayout->addWidget(metaLabel, 0, isMine ? Qt::AlignRight : Qt::AlignLeft);

    // 气泡对齐：自己的消息靠右，对方消息靠左
    if (isMine) {
        outerLayout->addStretch();
        outerLayout->addWidget(innerWidget);

        contentLabel->setStyleSheet(
            "background-color: #147d7e; color: white; border-radius: 12px;"
            "padding: 8px 14px; font-size: 14px;");
    } else {
        outerLayout->addWidget(innerWidget);
        outerLayout->addStretch();

        contentLabel->setStyleSheet(
            "background-color: #ffffff; color: #243447; border: 1px solid #d8dee8; border-radius: 12px;"
            "padding: 8px 14px; font-size: 14px;");
    }

    return bubble;
}

void ChatPage::appendMessage(const Message& message)
{
    // 移除底部弹簧（如果存在）
    if (m_messageLayout->count() > 0) {
        QLayoutItem* lastItem = m_messageLayout->itemAt(m_messageLayout->count() - 1);
        if (lastItem && lastItem->spacerItem()) {
            m_messageLayout->removeItem(lastItem);
            delete lastItem;
        }
    }

    // 添加消息气泡
    QWidget* bubble = createMessageBubble(message);
    m_messageLayout->addWidget(bubble);

    // 重新添加底部弹簧
    m_messageLayout->addStretch();

    // 如果用户在底部附近，自动滚动到底部
    if (isNearBottom()) {
        scrollToBottom();
    }
}

void ChatPage::clearMessageBubbles()
{
    // 清空气泡状态标签映射
    m_bubbleStatusLabels.clear();

    // 移除所有消息气泡（保留底部弹簧）
    while (m_messageLayout->count() > 1) {
        QLayoutItem* item = m_messageLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void ChatPage::updateMessageStatusInBubble(const QString& messageId, MessageStatus status)
{
    auto it = m_bubbleStatusLabels.find(messageId);
    if (it == m_bubbleStatusLabels.end()) {
        return;
    }

    QLabel* metaLabel = it.value();
    if (!metaLabel) {
        return;
    }

    // 从当前文本中提取时间部分（前 5 个字符为 "HH:mm"）
    QString currentText = metaLabel->text();
    QString timePart = currentText.left(5);

    QString indicator;
    switch (status) {
    case MessageStatus::Sending:   indicator = QStringLiteral(" ⏳"); break;
    case MessageStatus::Sent:      indicator = QStringLiteral(" ✓"); break;
    case MessageStatus::Delivered: indicator = QStringLiteral(" ✓✓"); break;
    case MessageStatus::Failed:    indicator = QStringLiteral(" ✗"); break;
    }

    metaLabel->setText(timePart + indicator);

    // 失败状态用红色文本
    if (status == MessageStatus::Failed) {
        metaLabel->setStyleSheet("font-size: 10px; color: #e74c3c; background: transparent;");
    }
}

// ---- Conversation List Item ----

QWidget* ChatPage::createConversationItemWidget(const Conversation& conv)
{
    auto* widget = new QWidget(m_conversationList);
    auto* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(8);

    // 左侧文字区域
    auto* textLayout = new QVBoxLayout();
    textLayout->setSpacing(3);

    // 对等体名称优先来自 core 层 peer 缓存，避免历史会话只显示长 peerId。
    QString displayName = conv.peerId;
    if (m_signalService) {
        const std::optional<PeerInfo> peer = m_signalService->peerById(conv.peerId);
        if (peer.has_value() && !peer->displayName.trimmed().isEmpty()) {
            displayName = peer->displayName;
        }
    }

    if (displayName.startsWith("peer_") && displayName.length() > 30) {
        displayName = displayName.left(12) + "...";
    }

    auto* nameLabel = new QLabel(displayName, widget);
    nameLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #333;");

    // 最后消息预览（截断长文本）
    QString preview = conv.lastMessage;
    if (preview.length() > 30) {
        preview = preview.left(30) + "...";
    }
    auto* previewLabel = new QLabel(preview, widget);
    previewLabel->setStyleSheet("font-size: 12px; color: #999;");
    previewLabel->setTextFormat(Qt::PlainText);

    textLayout->addWidget(nameLabel);
    textLayout->addWidget(previewLabel);

    layout->addLayout(textLayout, 1);

    // 右侧：时间 + 未读徽章
    auto* rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(4);
    rightLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // 时间戳
    QString timeStr;
    if (conv.lastMessageAt.isValid()) {
        QDateTime now = QDateTime::currentDateTime();
        QDate msgDate = conv.lastMessageAt.toLocalTime().date();
        if (msgDate == now.date()) {
            timeStr = conv.lastMessageAt.toLocalTime().toString("HH:mm");
        } else {
            timeStr = conv.lastMessageAt.toLocalTime().toString("MM-dd");
        }
    }
    auto* timeLabel = new QLabel(timeStr, widget);
    timeLabel->setStyleSheet("font-size: 11px; color: #bbb;");
    timeLabel->setAlignment(Qt::AlignRight);

    rightLayout->addWidget(timeLabel);

    // 未读计数徽章
    if (conv.unreadCount > 0) {
        QString countStr = conv.unreadCount > 99
            ? QStringLiteral("99+")
            : QString::number(conv.unreadCount);
        auto* badge = new QLabel(countStr, widget);
        badge->setFixedSize(22, 18);
        badge->setAlignment(Qt::AlignCenter);
        badge->setStyleSheet(
            "background-color: #e74c3c; color: white; border-radius: 9px;"
            "font-size: 10px; font-weight: bold;");
        rightLayout->addWidget(badge, 0, Qt::AlignRight);
    }

    layout->addLayout(rightLayout);

    return widget;
}

// ---- Helpers ----

void ChatPage::scrollToBottom()
{
    QScrollBar* bar = m_messageScroll->verticalScrollBar();
    if (bar) {
        bar->setValue(bar->maximum());
    }
}

bool ChatPage::isNearBottom() const
{
    QScrollBar* bar = m_messageScroll->verticalScrollBar();
    if (!bar) {
        return true;
    }
    return (bar->maximum() - bar->value()) <= kScrollNearBottomThreshold;
}

bool ChatPage::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::DragEnter) {
        auto* dragEvent = static_cast<QDragEnterEvent*>(event);
        if (dragEvent->mimeData() && dragEvent->mimeData()->hasUrls()) {
            dragEvent->acceptProposedAction();
            return true;
        }
    }

    if (event->type() == QEvent::Drop) {
        auto* dropEvent = static_cast<QDropEvent*>(event);
        if (dropEvent->mimeData() && handleDroppedUrls(dropEvent->mimeData()->urls())) {
            dropEvent->acceptProposedAction();
            return true;
        }
    }

    // 处理输入框的 Enter / Shift+Enter 键
    if (obj == m_inputEdit && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (keyEvent->modifiers() & Qt::ShiftModifier) {
                // Shift+Enter：插入换行（默认行为）
                return false;
            }
            // Enter：发送消息
            onSendClicked();
            return true;  // 事件已处理，不继续传递
        }
    }
    return QWidget::eventFilter(obj, event);
}

bool ChatPage::handleDroppedUrls(const QList<QUrl>& urls)
{
    if (urls.isEmpty()) {
        return false;
    }

    if (!m_courierService || m_activePeer.peerId.isEmpty()) {
        QMessageBox::information(this,
                                 QStringLiteral("发送文件"),
                                 QStringLiteral("请先选择一个在线会话"));
        return true;
    }
    if (!m_activePeer.online) {
        QMessageBox::information(this,
                                 QStringLiteral("发送文件"),
                                 QStringLiteral("对方当前离线，无法发送文件"));
        return true;
    }
    if (urls.size() != 1) {
        QMessageBox::information(this,
                                 QStringLiteral("发送文件"),
                                 QStringLiteral("当前仅支持拖入单个文件"));
        return true;
    }

    const QUrl url = urls.first();
    if (!url.isLocalFile()) {
        QMessageBox::information(this,
                                 QStringLiteral("发送文件"),
                                 QStringLiteral("仅支持本地文件"));
        return true;
    }

    const QString filePath = url.toLocalFile();
    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        QMessageBox::information(this,
                                 QStringLiteral("发送文件"),
                                 QStringLiteral("当前仅支持拖入单个文件"));
        return true;
    }

    m_courierService->sendFile(m_activePeer, filePath);
    return true;
}

} // namespace FengSui
