// ModelEnums.h
// 集中定义 models 层的枚举，并统一注册命名空间级反射（Q_NAMESPACE + Q_ENUM_NS），
// 使这些枚举可被 QML 反射（如 QML 中写 MessageStatus.Delivered）。
//
// 为什么集中到一个头：Q_NAMESPACE 在同一命名空间内只能出现一次，否则同一编译单元
// 同时包含多个带 Q_NAMESPACE 的头会产生 FengSui::staticMetaObject 重定义（ODR 冲突）。
// 因此枚举定义从各自的 models 头迁到此处集中管理，各 models 头改为 include 本文件。
// 限定名（如 MessageType::Text、TransferStatus::Completed）保持不变，core/storage 零改动。

#pragma once

#include <QObject>       // Q_NAMESPACE / Q_ENUM_NS 需要

namespace FengSui {

Q_NAMESPACE

// —— 消息（Message）——

// 消息类型
enum class MessageType {
    Text,           // 文本消息
    FileRequest,    // 文件传输请求（聊天窗口显示为"发送了 xxx.pdf"）
    System          // 系统消息（如"对方已接收文件"）
};
Q_ENUM_NS(MessageType)

// 消息投递状态
enum class MessageStatus {
    Sending,        // 正在发送
    Sent,           // 已发送（等待对方 ack）
    Delivered,      // 对方已确认收到
    Failed          // 发送失败
};
Q_ENUM_NS(MessageStatus)

// —— 文件传输（TransferTask）——

// 传输方向
enum class TransferDirection {
    Upload,     // 我发出
    Download    // 我接收
};
Q_ENUM_NS(TransferDirection)

// 传输状态
enum class TransferStatus {
    Waiting,        // 等待对方接受
    Rejected,       // 对方拒绝
    Transferring,   // 传输中
    Completed,      // 传输完成
    Failed,         // 传输失败
    Cancelled       // 已取消
};
Q_ENUM_NS(TransferStatus)

// —— 网卡（NetworkInterfaceInfo）——

// 网卡类型分类
enum class InterfaceType {
    Physical,  // 物理网卡（Ethernet、Wi-Fi）
    Virtual,   // 虚拟网卡（VMware、Hyper-V、WSL、ZeroTier 等）
    Loopback,  // 回环接口
    LinkLocal, // 链路本地地址（169.254.0.0/16）
    Public,    // 公网地址（非 RFC1918）
    Unknown    // 无法分类
};
Q_ENUM_NS(InterfaceType)

} // namespace FengSui
