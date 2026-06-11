// CourierService.h
// 文件传输编排服务：管理传输请求、接受/拒绝、分块收发、SHA-256 校验和进度上报。
// 不包含 QWidget，通过 signal/slot 与 UI 交互。
// 复用 SignalService 管理的 TCP 连接发送控制消息和二进制 chunk。

#pragma once

#include "models/PeerInfo.h"
#include "models/TransferTask.h"

#include <QHash>
#include <QObject>
#include <QString>

class QCryptographicHash;
class QFile;

namespace FengSui {

class AppSettings;
class SignalService;
class TcpConnection;
class TransferRepository;

// 单个传输任务的内部上下文。
// 仅在 CourierService 内部使用，外部通过 TransferTask 模型了解状态。
struct TransferContext {
    TransferTask task;                  // 传输任务元信息
    PeerInfo     peer;                  // 对端设备信息
    QFile*       file = nullptr;        // 本地文件句柄（发送时读，接收时写）
    QCryptographicHash* hasher = nullptr; // SHA-256 累加器
    quint32      nextChunkIndex = 0;    // 下一块的序号
    quint32      chunkSize = 8388608;   // 每块字节数（8MB 默认，可协商）
    bool         isOutgoing = false;    // true=我发出，false=我接收
};

// 文件传输编排服务。
// 为每个传输任务维护上下文，管理从 transfer.request 到 transfer.complete 的完整生命周期。
// 线程安全性：所有操作在主线程执行，QFile 操作通过 QIODevice 的非阻塞模式。
class CourierService : public QObject {
    Q_OBJECT

public:
    // 创建文件传输编排服务。
    // parent: Qt 父对象，用于资源释放。
    explicit CourierService(QObject* parent = nullptr);

    // 销毁传输编排服务。
    // 析构时清理所有活跃传输任务（关闭文件句柄、释放资源）。
    ~CourierService() override;

    // ---- 依赖注入 ----

    // 注入消息通道服务。由 Application 在初始化时调用。
    void setSignalService(SignalService* service);

    // 注入应用设置。用于读取默认下载目录等本地配置。
    void setAppSettings(AppSettings* settings);

    // 注入传输任务存储仓库。由 Application 在初始化时调用。
    void setTransferRepository(TransferRepository* repo);

    // 注入本机 peer_id。
    void setLocalPeerId(const QString& peerId);

    // ---- 传输操作 ----

    // 向指定对等体发送文件。
    // peer: 目标对等体信息（需包含 ip、port、peerId）。
    // filePath: 本地文件的绝对路径。
    // 返回值：传输任务 ID，调用方用于追踪进度。
    // 线程安全性：文件 I/O 在事件循环中分块执行，不阻塞 UI 线程。
    QString sendFile(const PeerInfo& peer, const QString& filePath);

    // 向指定对等体发送文件夹（递归打包传输）。
    // peer: 目标对等体信息。
    // dirPath: 本地文件夹的绝对路径。
    // 返回值：传输任务 ID。
    QString sendFolder(const PeerInfo& peer, const QString& dirPath);

    // 接受传输请求。
    // transferId: 被接受的传输任务 ID。
    void acceptTransfer(const QString& transferId);

    // 拒绝传输请求。
    // transferId: 被拒绝的传输任务 ID。
    // reason: 拒绝原因（可选，会发给请求方）。
    void rejectTransfer(const QString& transferId, const QString& reason = QString());

    // 取消正在进行的传输。
    // transferId: 被取消的传输任务 ID。
    void cancelTransfer(const QString& transferId);

    // 对等体连接断开时标记相关活跃传输失败。
    void handlePeerDisconnected(const QString& peerId);

    // ---- 消息入口（由 SignalService 调用） ----

    // 处理收到的 transfer.* 族 JSON 消息。
    // connection: 消息来源连接（用于发送 chunk 数据或回复）。
    // message: 解析后的协议消息。
    void handleTransferMessage(TcpConnection* connection,
                               const QJsonObject& message);

    // 处理收到的二进制 chunk 数据。
    // transferId: 传输任务 ID。
    // chunkIndex: 块序号。
    // data: 块数据载荷。
    void handleChunkData(const QString& transferId,
                         quint32 chunkIndex,
                         const QByteArray& data);

    // ---- 查询 ----

    // 获取所有传输任务快照（从 TransferRepository 加载）。
    QList<TransferTask> allTasks() const;

    // 获取进行中的传输任务。
    QList<TransferTask> activeTasks() const;

signals:
    // 收到新的传输请求（接收方 UI 弹出接受/拒绝对话框）。
    void transferRequested(const TransferTask& task);

    // 传输请求被对方接受。
    void transferAccepted(const QString& transferId);

    // 传输请求被对方拒绝。
    void transferRejected(const QString& transferId, const QString& reason);

    // 传输进度更新。
    // transferId: 传输任务 ID。
    // bytesTransferred: 已传输字节数。
    // totalBytes: 总字节数。
    void transferProgress(const QString& transferId,
                          qint64 bytesTransferred,
                          qint64 totalBytes);

    // 传输完成。
    void transferCompleted(const QString& transferId);

    // 传输失败。
    // errorMessage: 可读错误信息。
    void transferFailed(const QString& transferId, const QString& errorMessage);

private:
    // 处理 transfer.request 消息。
    void handleTransferRequest(TcpConnection* connection,
                               const QJsonObject& message);

    // 处理 transfer.accept 消息（发送方收到接受回复）。
    void handleTransferAccept(const QJsonObject& message);

    // 处理 transfer.reject 消息（发送方收到拒绝回复）。
    void handleTransferReject(const QJsonObject& message);

    // 处理 transfer.complete 消息（接收方收到完成通知）。
    void handleTransferComplete(const QJsonObject& message);

    // 处理 transfer.error 消息。
    void handleTransferError(const QJsonObject& message);

    // 开始分块发送（当收到 accept 后调用）。
    void startSendingChunks(const QString& transferId);

    // 发送下一个数据块。
    void sendNextChunk(const QString& transferId);

    // 完成传输（校验 SHA-256、关闭文件、更新状态）。
    void finishTransfer(const QString& transferId, bool success,
                        const QString& errorMessage = QString());

    // 生成传输任务 ID。
    QString generateTransferId() const;

    // 计算文件 SHA-256 校验和（同步，在 sendFile 时调用一次）。
    static QString computeFileSha256(const QString& filePath);

    // 查找与 transferId 关联的活跃 TcpConnection。
    TcpConnection* findConnectionForTransfer(const QString& transferId) const;

    // 发送 transfer.error 给对端（尽力发送）。
    void sendTransferError(const QString& transferId,
                           const QString& errorCode,
                           const QString& errorMessage);

    SignalService*       m_signalService = nullptr;
    AppSettings*         m_settings = nullptr;
    TransferRepository*  m_transferRepo = nullptr;
    QString              m_localPeerId;

    // transferId → TransferContext 映射（仅活跃传输，完成后移除）
    QHash<QString, TransferContext> m_activeTransfers;
};

} // namespace FengSui
