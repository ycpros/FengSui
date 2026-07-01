// SignalService.h
// TCP 消息通道编排服务：管理 TcpServer 和所有 TcpConnection，负责消息路由、
// ACK 自动回复、重复连接解决、待发送队列。
// 不包含界面控件，仅通过 signal/slot 与 UI 层交互。

#pragma once

#include "models/Conversation.h"
#include "models/Message.h"
#include "models/NetworkPolicy.h"
#include "models/PeerInfo.h"

#include <QHash>
#include <QHostAddress>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <optional>

namespace FengSui {

class AppSettings;
class ConversationRepository;
class CourierService;
class MessageRepository;
class NetworkPolicy;
class ShareService;
class TcpConnection;
class TcpServer;

// TCP 消息通道编排服务。
// 为每个已知对等体维护一条 TcpConnection，提供 sendMessage() 发送接口，
// 通过 messageReceived 信号向上层（UI）投递收到的消息。
// 线程安全性：所有操作在主线程执行，依赖 Qt 事件驱动的非阻塞 I/O。
class SignalService : public QObject {
    Q_OBJECT

public:
    // 创建消息通道编排服务。
    // settings: 应用设置实例，提供 peer_id 和 listen_port，生命周期必须长于本对象。
    // parent: Qt 父对象，用于资源释放。
    explicit SignalService(AppSettings* settings,
                           NetworkPolicy* networkPolicy = nullptr,
                           QObject* parent = nullptr);

    // 销毁消息通道服务。
    // 析构时自动停止 TcpServer 并关闭所有连接。
    ~SignalService() override;

    // 启动 TCP 消息服务（监听端口 + 准备接受连接）。
    // errorOut: 启动失败时写入可读错误信息。
    // 返回值：TcpServer 成功绑定端口时返回 true。
    // 线程安全性：仅在对象所属线程调用。
    bool start(QString& errorOut);

    // 启动 TCP 消息服务并绑定到指定地址。
    // bindAddress: 显式监听地址；测试可传 127.0.0.1，生产路径由 start(errorOut) 自动选择。
    // errorOut: 启动失败时写入可读错误信息。
    // 返回值：TcpServer 成功绑定端口时返回 true。
    // 线程安全性：仅在对象所属线程调用。
    bool start(const QHostAddress& bindAddress, QString& errorOut);

    void setNetworkPolicy(NetworkPolicy* networkPolicy);

    // 停止 TCP 消息服务，关闭所有活动连接。
    // 线程安全性：仅在对象所属线程调用。
    void stop();

    // 返回消息服务是否正在运行。
    bool isRunning() const;

    // 发送文本消息到指定对等体。
    // peer: 目标对等体信息（需包含 ip、port、peerId）。
    // content: 文本消息正文。
    // 返回值：生成的 messageId，调用方用于追踪发送状态。
    QString sendMessage(const PeerInfo& peer, const QString& content);

    // 查询是否与指定对等体保持 TCP 连接。
    // peerId: 对等体标识。
    bool isConnectedTo(const QString& peerId) const;

    // 发送任意 JSON 控制消息到指定对等体。
    // peer: 目标对等体信息（需包含 ip、port、peerId）。
    // message: 完整的协议 JSON 消息（调用方负责填充 type 等必要字段）。
    // 返回值：用于发送的 TcpConnection*，若连接正在建立中则返回 nullptr（消息已入队）。
    TcpConnection* sendJsonMessage(const PeerInfo& peer,
                                   const QJsonObject& message);

    // 获取与指定对等体的活跃 TCP 连接。
    // peerId: 对等体标识。
    // 返回值：连接存在时返回 TcpConnection*，否则返回 nullptr。
    TcpConnection* connectionTo(const QString& peerId) const;

    // ---- 存储层与子服务注入 ----

    // 注入会话存储仓库。由 Application 在初始化时调用。
    void setConversationRepository(ConversationRepository* repo);

    // 注入消息存储仓库。由 Application 在初始化时调用。
    void setMessageRepository(MessageRepository* repo);

    // 注入文件传输编排服务。由 Application 在初始化时调用。
    void setCourierService(CourierService* service);

    // 注入共享目录服务。由 Application 在初始化时调用。
    void setShareService(ShareService* service);

    // 获取已注入的会话仓库（保留给 core 内部和测试辅助，UI 层不应直接调用）。
    ConversationRepository* conversationRepository() const;

    // ---- 存储查询代理（委托给 Repository） ----

    // 打开或创建与指定对等体的会话。
    // peer: 对端设备信息；包含 ip/port 时会同步到 peers 表，便于历史会话再次发送。
    // 返回值：会话元信息；失败时 conversationId 为空。
    Conversation openConversation(const PeerInfo& peer);

    // 获取所有会话列表。
    QList<Conversation> conversations() const;

    // 按 peerId 获取对等体信息。
    std::optional<PeerInfo> peerById(const QString& peerId) const;

    // 根据会话 ID 获取对等体信息。
    std::optional<PeerInfo> peerForConversation(const QString& conversationId) const;

    // 获取指定会话的消息历史。
    QList<Message> messageHistory(const QString& conversationId,
                                  int limit = 50,
                                  int offset = 0) const;

    // 将指定会话的未读计数归零。
    void markConversationRead(const QString& conversationId);

signals:
    // 收到完整协议消息。
    // message: 解析后的 JSON 消息对象（message.text / message.ack / message.system）。
    void messageReceived(const QJsonObject& message);

    // 与指定对等体的 TCP 连接已建立。
    void peerConnected(const QString& peerId);

    // 与指定对等体的 TCP 连接已断开。
    void peerDisconnected(const QString& peerId);

    // 非致命错误通知。
    void errorOccurred(const QString& errorMessage);

    // 消息已持久化到数据库（含发送和接收的消息）。
    // UI 层据此更新消息气泡列表和会话列表。
    void messageStored(const Message& message);

    // 消息投递状态变更（sending → sent → delivered → failed）。
    // messageId: 消息 ID；status: MessageStatus 枚举的 int 值。
    void messageStatusChanged(const QString& messageId, int status);

    // 会话元信息变更（最后消息、未读计数等）。
    // UI 层据此刷新会话列表。
    void conversationUpdated(const QString& conversationId);

private slots:
    // TcpServer 有新入站连接时调用。
    void onNewIncomingConnection(TcpConnection* connection);

    // TcpConnection 收到完整消息时调用。
    void onConnectionMessage(const QJsonObject& message);

    // TcpConnection 断开时调用。
    void onConnectionDisconnected();

    // TcpConnection 发生错误时调用。
    void onConnectionError(const QString& errorMessage);

    // 出站连接建立成功时调用。
    void onOutgoingConnected();

private:
    // 将连接注册到已知对等体映射。
    void registerConnection(const QString& peerId, TcpConnection* connection);

    // 从已知对等体映射中移除连接。
    void unregisterConnection(const QString& peerId);

    // 查找或创建到指定对等体的 TCP 连接。
    // 已有连接直接返回；无连接时发起出站连接并返回 nullptr（消息进入待发送队列）。
    TcpConnection* ensureConnection(const PeerInfo& peer);

    // 查找已注册的对等体连接。
    TcpConnection* findConnection(const QString& peerId) const;

    // 生成唯一消息 ID。
    QString generateMessageId() const;

    // 向指定 messageId 的发送方回复 ACK。
    void sendAck(const QString& messageId, const QString& status);

    // 对等体身份识别：从入站连接首条消息的 from 字段识别。
    void identifyIncomingConnection(TcpConnection* connection,
                                    const QJsonObject& firstMessage);

    // 处理重复连接：双方同时发起出站时，peerId 字典序较小者保留连接。
    // 返回 true 表示传入的连接应被保留。
    bool resolveDuplicateConnection(const QString& peerId,
                                    TcpConnection* newConnection);

    // 刷新指定对等体的待发送消息队列。
    void flushPendingMessages(const QString& peerId);

    // 将指定对等体的待发送文本消息标记为失败并清空队列。
    void failPendingMessages(const QString& peerId, const QString& errorMessage);

    QList<BindEndpoint> buildBindEndpoints() const;
    void connectConnectionServices(TcpConnection* connection);

    AppSettings* m_settings = nullptr;
    NetworkPolicy* m_networkPolicy = nullptr;
    TcpServer* m_server = nullptr;
    QString m_localPeerId;
    bool m_running = false;

    // peerId → TcpConnection* 映射（已知对等体的活跃连接）
    QHash<QString, TcpConnection*> m_connections;

    // 尚未识别对等体身份的入站连接
    QList<TcpConnection*> m_pendingConnections;

    // 出站连接建立中暂存的消息：peerId → 待发送 JSON 列表
    QHash<QString, QList<QJsonObject>> m_pendingMessages;

    // 存储层与子服务依赖（由 Application 注入，可选）
    ConversationRepository* m_conversationRepo = nullptr;
    MessageRepository*      m_messageRepo = nullptr;
    CourierService*         m_courierService = nullptr;
    QObject*                m_shareService = nullptr;
};

} // namespace FengSui
