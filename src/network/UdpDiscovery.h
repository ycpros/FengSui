// UdpDiscovery.h
// UDP 组播收发封装：发送 presence.hello/heartbeat/goodbye，接收其他设备广播。
// 任务 006 中实现。

#pragma once

#include <QHostAddress>
#include <QJsonObject>
#include <QObject>
#include <QString>

class QUdpSocket;

namespace FengSui {

// UdpDiscovery 只负责 UDP 多播 socket 和 JSON 编解码。
// 业务语义由 BeaconService 处理，收到有效 JSON 后通过 Qt 信号通知上层。
// 线程安全性：QUdpSocket 运行在对象所属线程，当前设计为主线程使用。
class UdpDiscovery : public QObject {
    Q_OBJECT

public:
    // 创建 UDP 发现收发器。
    // parent: Qt 父对象，用于资源释放。
    // 线程安全性：仅在主线程或同一 QObject 所属线程构造。
    explicit UdpDiscovery(QObject* parent = nullptr);

    // 销毁 UDP 发现收发器。
    // 析构时会调用 stop()，关闭 socket 并退出多播组。
    // 线程安全性：仅在对象所属线程销毁。
    ~UdpDiscovery() override;

    // 启动 UDP 多播监听。
    // errorOut: 启动失败时写入可读错误信息。
    // 返回值：socket 已绑定并加入多播组时返回 true。
    // 线程安全性：仅在对象所属线程调用。
    bool start(QString& errorOut);

    // 停止监听并释放 socket。
    // 线程安全性：仅在对象所属线程调用。
    void stop();

    // 返回当前 UDP socket 是否处于运行状态。
    // 返回值：start() 成功后为 true，stop() 后为 false。
    // 线程安全性：仅在对象所属线程调用。
    bool isRunning() const;

    // 发送 presence.hello 报文。
    // payload: 除 type 外的 hello 字段；函数会强制写入 type。
    // errorOut: 发送失败时写入可读错误信息。
    // 返回值：完整写入 UDP 数据报时返回 true。
    // 线程安全性：仅在对象所属线程调用。
    bool sendHello(const QJsonObject& payload, QString& errorOut);

    // 发送 presence.heartbeat 报文。
    // peerId: 本机 peer_id，不能为空。
    // errorOut: 发送失败时写入可读错误信息。
    // 返回值：完整写入 UDP 数据报时返回 true。
    // 线程安全性：仅在对象所属线程调用。
    bool sendHeartbeat(const QString& peerId, QString& errorOut);

    // 发送 presence.goodbye 报文。
    // peerId: 本机 peer_id，不能为空。
    // errorOut: 发送失败时写入可读错误信息。
    // 返回值：完整写入 UDP 数据报时返回 true。
    // 线程安全性：仅在对象所属线程调用。
    bool sendGoodbye(const QString& peerId, QString& errorOut);

signals:
    // 收到有效 JSON 报文。
    // senderAddress/senderPort 为 UDP 源地址，用于补全对端 IP。
    // message: 已解析为对象的报文内容。
    void datagramReceived(const QJsonObject& message,
                          const QHostAddress& senderAddress,
                          quint16 senderPort);

    // UDP 层错误通知。
    // errorMessage: socket 或 JSON 解析失败的可读错误信息。
    void errorOccurred(const QString& errorMessage);

private:
    // 读取所有待处理 UDP 报文。
    void onReadyRead();

    // 发送 JSON 对象到发现多播地址。
    bool sendMessage(const QJsonObject& message, QString& errorOut);

    QUdpSocket* m_socket = nullptr;
    bool m_running = false;
};

} // namespace FengSui
