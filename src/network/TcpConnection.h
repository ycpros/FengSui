// TcpConnection.h
// 单条 TCP 连接封装：读缓冲累积、帧提取循环、发送/接收 JSON。
// 不处理业务语义，仅负责 wire format 层的收发和连接状态通知。

#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>

class QTcpSocket;

namespace FengSui {

// TCP 连接封装。
// 维护单个 QTcpSocket 的完整生命周期：连接、读帧、写帧、断开。
// 入站连接通过 setSocket() 注入已 accept 的 socket（由 TcpServer 创建），
// 出站连接通过 connectToHost() 自行创建 socket。
// 线程安全性：QTcpSocket 运行在对象所属线程，当前设计为主线程使用。
class TcpConnection : public QObject {
    Q_OBJECT

public:
    // 创建 TCP 连接封装。
    // parent: Qt 父对象，用于资源释放。
    explicit TcpConnection(QObject* parent = nullptr);

    // 销毁连接封装。
    // 析构时断开 socket 并释放资源。
    ~TcpConnection() override;

    // 接管已建立的 QTcpSocket（用于入站连接）。
    // socket: TcpServer 调用 nextPendingConnection() 返回的已连接 socket。
    // 调用后本对象开始监听该 socket 的可读事件。
    void setSocket(QTcpSocket* socket);

    // 发起出站 TCP 连接。
    // host: 对端 IPv4 地址。
    // port: 对端 TCP 监听端口。
    void connectToHost(const QString& host, quint16 port);

    // 发送 JSON 消息（text/ack/system/transfer.request 等控制消息）。
    // message: 完整协议消息，调用方负责保证 type 等必要字段存在。
    // 返回值：完整写入 socket 缓冲区时返回 true。
    bool sendMessage(const QJsonObject& message);

    // 发送二进制 chunk 帧（transfer.chunk 的数据块）。
    // chunkFrame: Protocol::buildChunkFrame() 输出的完整二进制帧。
    // 返回值：完整写入 socket 缓冲区时返回 true。
    bool sendBinaryChunk(const QByteArray& chunkFrame);

    // 获取已关联的对等体标识。
    // 入站连接在收到首条消息前为空，出站连接在 connectToHost 前由调用方设置。
    QString peerId() const;

    // 设置对等体标识。
    void setPeerId(const QString& peerId);

    // 返回 socket 是否处于已连接状态。
    bool isConnected() const;

    // 主动断开连接。
    void disconnect();

signals:
    // 收到完整 JSON 消息（message.text / message.ack / transfer.request 等控制消息）。
    void messageReceived(const QJsonObject& message);

    // 收到完整二进制 chunk 帧（transfer.chunk 数据块）。
    // transferId: 传输任务 ID。
    // chunkIndex: 块序号，从 0 开始。
    // data: 块数据载荷。
    void binaryChunkReceived(const QString& transferId,
                             quint32 chunkIndex,
                             const QByteArray& data);

    // 出站连接成功建立。
    void connected();

    // 连接已断开（主动或被动）。
    void disconnected();

    // 连接层错误（socket 错误或帧解析错误）。
    void errorOccurred(const QString& errorMessage);

private slots:
    // socket 有数据可读时调用。
    void onReadyRead();

    // 出站连接建立成功。
    void onSocketConnected();

    // socket 断开。
    void onSocketDisconnected();

    // socket 层错误。
    void onSocketError();

private:
    // 从 m_readBuffer 中循环提取完整帧并发射 messageReceived。
    void processReadBuffer();

    QTcpSocket* m_socket = nullptr;
    QByteArray m_readBuffer;
    QString m_peerId;
    bool m_ownsSocket = false;
};

} // namespace FengSui

Q_DECLARE_METATYPE(FengSui::TcpConnection*)
