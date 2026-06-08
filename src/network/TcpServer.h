// TcpServer.h
// TCP 监听器：封装 QTcpServer，接受入站连接并创建 TcpConnection。
// 设计模式与 UdpDiscovery 一致：thin wrapper，业务逻辑留在 core/ 层。

#pragma once

#include <QHostAddress>
#include <QObject>
#include <QString>

class QTcpServer;

namespace FengSui {

class TcpConnection;

// TCP 监听器。
// 绑定指定端口，接受入站 TCP 连接，为每个连接创建 TcpConnection 并发出 newConnection 信号。
// 线程安全性：QTcpServer 运行在对象所属线程，当前设计为主线程使用。
class TcpServer : public QObject {
    Q_OBJECT

public:
    // 创建 TCP 监听器。
    // parent: Qt 父对象，用于资源释放。
    explicit TcpServer(QObject* parent = nullptr);

    // 销毁监听器。
    // 析构时自动关闭所有连接并释放 socket。
    ~TcpServer() override;

    // 启动监听，兼容旧调用路径。
    // port: 绑定端口；默认绑定 localhost，避免无意暴露到所有网卡。
    // errorOut: 启动失败时写入可读错误信息。
    // 返回值：成功绑定并开始监听时返回 true。
    // 线程安全性：仅在对象所属线程调用。
    bool start(quint16 port, QString& errorOut);

    // 启动监听。
    // bindAddress: 显式绑定地址，由 core 层按当前网络策略选择。
    // port: 绑定端口，使用 ShareAddress + ReuseAddressHint 允许多实例本地调试。
    // errorOut: 启动失败时写入可读错误信息。
    // 返回值：成功绑定并开始监听时返回 true。
    // 线程安全性：仅在对象所属线程调用。
    bool start(const QHostAddress& bindAddress, quint16 port, QString& errorOut);

    // 停止监听并关闭所有活动连接。
    // 线程安全性：仅在对象所属线程调用。
    void stop();

    // 返回监听器是否正在运行。
    bool isRunning() const;

    // 返回当前绑定的端口号。
    quint16 port() const;

signals:
    // 有新入站连接建立。
    // connection: 已封装的 TcpConnection，调用方负责关联 peer 身份。
    void newConnection(TcpConnection* connection);

    // 监听器级错误。
    void errorOccurred(const QString& errorMessage);

private slots:
    // QTcpServer 有新连接待 accept 时调用。
    void onNewConnection();

private:
    QTcpServer* m_server = nullptr;
    QHostAddress m_bindAddress;
    quint16 m_port = 0;
    bool m_running = false;
};

} // namespace FengSui
