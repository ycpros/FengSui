// Protocol.h
// TCP 消息协议的纯静态工具类：wire format 打包/解包、消息类型检查、消息构建器。
// 不继承 QObject，无状态，所有方法为静态方法，可被所有层安全 include。

#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>

namespace FengSui {

// 协议工具类。
// 封装 06_protocol.md 定义的 wire format：[4 字节大端长度][Compact JSON 正文]。
// 同时提供消息类型检查与常见消息构建器，减少各层重复构造 JSON 的风险。
class Protocol {
public:
    Protocol() = delete;

    // ---- Wire Format ----

    // 将 JSON 消息打包为 wire format 字节数组。
    // message: 待发送的 JSON 对象，调用方负责保证字段完整。
    // 返回值：[4 字节 BE 长度][Compact JSON]，失败时返回空 QByteArray。
    static QByteArray pack(const QJsonObject& message);

    // 尝试从流式读缓冲中提取一个完整帧。
    // buffer: 累积的读缓冲，成功提取时会移除已消费的字节。
    // frameOut: 提取到的帧载荷（不含 4 字节长度头），仅成功时写入。
    // errorOut: 帧格式错误时写入可读错误信息。
    // 返回值：提取到完整帧返回 true；缓冲中帧数据不完整返回 false（无错误）。
    static bool extractFrame(QByteArray& buffer, QByteArray& frameOut,
                             QString& errorOut);

    // 将单个帧载荷（不含长度头）解包为 JSON 对象。
    // frame: extractFrame 输出的帧载荷。
    // messageOut: 成功时写入解析后的 JSON 对象。
    // errorOut: 解析失败时写入可读错误信息。
    // 返回值：解析成功返回 true。
    static bool unpack(const QByteArray& frame, QJsonObject& messageOut,
                       QString& errorOut);

    // ---- 消息类型检查 ----

    // 返回消息的 type 字段，不存在时返回空字符串。
    static QString messageType(const QJsonObject& message);

    // 判断消息是否为 message.text 类型。
    static bool isTextMessage(const QJsonObject& message);

    // 判断消息是否为 message.ack 类型。
    static bool isAck(const QJsonObject& message);

    // 判断消息是否为 message.system 类型。
    static bool isSystemMessage(const QJsonObject& message);

    // 判断消息是否为 transfer.request 类型。
    static bool isTransferRequest(const QJsonObject& message);

    // 判断消息是否为 transfer.accept 类型。
    static bool isTransferAccept(const QJsonObject& message);

    // 判断消息是否为 transfer.reject 类型。
    static bool isTransferReject(const QJsonObject& message);

    // 判断消息是否为 transfer.chunk 类型。
    static bool isTransferChunk(const QJsonObject& message);

    // 判断消息是否为 transfer.complete 类型。
    static bool isTransferComplete(const QJsonObject& message);

    // 判断消息是否为 transfer.error 类型。
    static bool isTransferError(const QJsonObject& message);

    // 判断消息是否属于 transfer.* 族消息（含 chunk）。
    // 用于在 TcpConnection 层快速判断是否转发给 CourierService。
    static bool isTransferMessage(const QJsonObject& message);

    // ---- 消息构建器 ----

    // 构建 message.text JSON 对象。
    // messageId: 消息唯一标识，调用方通过 QUuid 生成。
    // from: 发送方 peer_id。
    // to: 接收方 peer_id。
    // content: 文本消息正文。
    static QJsonObject buildTextMessage(const QString& messageId,
                                        const QString& from,
                                        const QString& to,
                                        const QString& content);

    // 构建 message.ack JSON 对象。
    // messageId: 被确认的消息 ID。
    // status: "delivered" 或 "failed"。
    static QJsonObject buildAck(const QString& messageId,
                                const QString& status);

    // 构建 message.system JSON 对象。
    // from: 发送方 peer_id。
    // to: 接收方 peer_id。
    // content: 系统消息正文。
    static QJsonObject buildSystemMessage(const QString& from,
                                          const QString& to,
                                          const QString& content);

    // ---- 文件传输协议构建器 ----

    // 构建 transfer.request JSON 对象。
    // transferId: 传输任务唯一标识。
    // from: 发送方 peer_id。
    // to: 接收方 peer_id。
    // fileName: 文件名称。
    // fileSize: 文件大小（字节）。
    // sha256: 文件 SHA-256 校验和（十六进制字符串）。
    // chunkSize: 每块字节数，默认 8MB。
    // isFolder: 是否为文件夹传输。
    static QJsonObject buildTransferRequest(const QString& transferId,
                                            const QString& from,
                                            const QString& to,
                                            const QString& fileName,
                                            qint64 fileSize,
                                            const QString& sha256,
                                            quint32 chunkSize = 8388608,
                                            bool isFolder = false);

    // 构建 transfer.accept JSON 对象。
    // transferId: 被接受的传输任务 ID。
    static QJsonObject buildTransferAccept(const QString& transferId);

    // 构建 transfer.reject JSON 对象。
    // transferId: 被拒绝的传输任务 ID。
    // reason: 拒绝原因（可选）。
    static QJsonObject buildTransferReject(const QString& transferId,
                                           const QString& reason = QString());

    // 构建 transfer.complete JSON 对象。
    // transferId: 完成的传输任务 ID。
    // sha256: 接收方计算的 SHA-256 校验和。
    static QJsonObject buildTransferComplete(const QString& transferId,
                                             const QString& sha256);

    // 构建 transfer.error JSON 对象。
    // transferId: 出错的传输任务 ID。
    // errorCode: 错误码（如 NETWORK_TIMEOUT、DISK_FULL 等）。
    // errorMessage: 可读错误信息。
    static QJsonObject buildTransferError(const QString& transferId,
                                          const QString& errorCode,
                                          const QString& errorMessage);

    // ---- 二进制 Chunk 帧序列化 ----

    // 构建 transfer.chunk 二进制帧。
    // transferId: 传输任务 ID。
    // chunkIndex: 块序号（从 0 开始）。
    // data: 块数据（最大 8MB）。
    // 返回值：按 [4B transfer_id 长度][transfer_id UTF-8][4B chunk_index][4B data 长度][data] 格式化的字节数组。
    static QByteArray buildChunkFrame(const QString& transferId,
                                      quint32 chunkIndex,
                                      const QByteArray& data);

    // 尝试从二进制帧载荷中解析 transfer.chunk 各字段。
    // frameData: buildChunkFrame 输出的帧数据。
    // transferIdOut: 成功时写入 transfer_id。
    // chunkIndexOut: 成功时写入 chunk_index。
    // dataOut: 成功时写入数据载荷。
    // errorOut: 解析失败时写入错误信息。
    // 返回值：解析成功返回 true。
    static bool parseChunkFrame(const QByteArray& frameData,
                                QString& transferIdOut,
                                quint32& chunkIndexOut,
                                QByteArray& dataOut,
                                QString& errorOut);

    // 判断给定字节数组是否可能是二进制 chunk 帧（首帧字节不是 JSON '{'）。
    static bool isLikelyChunkFrame(const QByteArray& frameData);

private:
    // 单帧载荷最大字节数（100 MB），超出视为恶意或错误数据。
    static constexpr quint32 kMaxFrameSize = 100 * 1024 * 1024;
};

} // namespace FengSui
