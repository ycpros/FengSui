// TcpProbe.h
// 轻量 TCP 连通性探针，供手动添加设备等场景使用。
// 内部阻塞式实现，仅适用于 modal 对话框等短时等待场景。

#pragma once

#include <QObject>
#include <QString>

namespace FengSui {

// TCP 可达性探针。
// 仅验证目标 IP:端口是否可达，不进行任何协议交互。
// 线程安全性：仅在主线程调用（内部使用 QEventLoop）。
class TcpProbe : public QObject {
    Q_OBJECT

public:
    explicit TcpProbe(QObject* parent = nullptr);

    // 尝试 TCP 连接到目标主机。
    // host: IPv4 地址或主机名。
    // port: 目标端口号。
    // timeoutMs: 超时毫秒数。
    // errorOut: 失败时写入中文错误描述。
    // 返回 true 表示 TCP 握手成功。
    static bool probe(const QString& host, quint16 port, int timeoutMs, QString& errorOut);
};

} // namespace FengSui
