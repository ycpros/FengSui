// Protocol.cpp
// 协议工具类实现：wire format 封帧/解帧、消息类型检查、消息构建。

#include "network/Protocol.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QtEndian>

namespace FengSui {

// ---- Wire Format ----

QByteArray Protocol::pack(const QJsonObject& message)
{
    const QByteArray json = QJsonDocument(message).toJson(QJsonDocument::Compact);
    if (json.isEmpty()) {
        return {};
    }

    const quint32 length = static_cast<quint32>(json.size());
    QByteArray frame;
    frame.resize(static_cast<int>(sizeof(quint32) + json.size()));

    // 写入 4 字节大端长度前缀
    qToBigEndian(length, reinterpret_cast<uchar*>(frame.data()));

    // 写入 Compact JSON 正文
    std::memcpy(frame.data() + sizeof(quint32), json.constData(),
                static_cast<size_t>(json.size()));

    return frame;
}

bool Protocol::extractFrame(QByteArray& buffer, QByteArray& frameOut,
                            QString& errorOut)
{
    // 至少需要 4 字节长度前缀才能读取帧长度
    if (buffer.size() < static_cast<int>(sizeof(quint32))) {
        return false;
    }

    const quint32 frameLength = qFromBigEndian<quint32>(
        reinterpret_cast<const uchar*>(buffer.constData()));

    // 长度合法性检查：0 字节帧无意义，超大帧可能是恶意数据
    if (frameLength == 0) {
        errorOut = QStringLiteral("Frame length is zero");
        buffer.remove(0, static_cast<int>(sizeof(quint32)));
        return false;
    }

    if (frameLength > kMaxFrameSize) {
        errorOut = QStringLiteral("Frame length %1 exceeds maximum %2")
                       .arg(frameLength)
                       .arg(kMaxFrameSize);
        // 无法恢复：丢弃整个缓冲区，防止后续数据错位
        buffer.clear();
        return false;
    }

    const int totalFrameSize = static_cast<int>(sizeof(quint32) + frameLength);
    if (buffer.size() < totalFrameSize) {
        // 帧数据尚未完整到达，等待更多数据
        return false;
    }

    // 提取帧载荷（不含长度头）
    frameOut = buffer.mid(static_cast<int>(sizeof(quint32)),
                          static_cast<int>(frameLength));

    // 从缓冲区移除已消费的字节
    buffer.remove(0, totalFrameSize);

    return true;
}

bool Protocol::unpack(const QByteArray& frame, QJsonObject& messageOut,
                      QString& errorOut)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(frame, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        errorOut = QStringLiteral("JSON parse error: %1").arg(parseError.errorString());
        return false;
    }

    if (!doc.isObject()) {
        errorOut = QStringLiteral("Frame JSON is not an object");
        return false;
    }

    messageOut = doc.object();
    return true;
}

// ---- 消息类型检查 ----

QString Protocol::messageType(const QJsonObject& message)
{
    return message.value(QStringLiteral("type")).toString();
}

bool Protocol::isTextMessage(const QJsonObject& message)
{
    return messageType(message) == QStringLiteral("message.text");
}

bool Protocol::isAck(const QJsonObject& message)
{
    return messageType(message) == QStringLiteral("message.ack");
}

bool Protocol::isSystemMessage(const QJsonObject& message)
{
    return messageType(message) == QStringLiteral("message.system");
}

bool Protocol::isTransferRequest(const QJsonObject& message)
{
    return messageType(message) == QStringLiteral("transfer.request");
}

bool Protocol::isTransferAccept(const QJsonObject& message)
{
    return messageType(message) == QStringLiteral("transfer.accept");
}

bool Protocol::isTransferReject(const QJsonObject& message)
{
    return messageType(message) == QStringLiteral("transfer.reject");
}

bool Protocol::isTransferChunk(const QJsonObject& message)
{
    return messageType(message) == QStringLiteral("transfer.chunk");
}

bool Protocol::isTransferComplete(const QJsonObject& message)
{
    return messageType(message) == QStringLiteral("transfer.complete");
}

bool Protocol::isTransferError(const QJsonObject& message)
{
    return messageType(message) == QStringLiteral("transfer.error");
}

bool Protocol::isTransferMessage(const QJsonObject& message)
{
    const QString type = messageType(message);
    return type == QStringLiteral("transfer.request")
        || type == QStringLiteral("transfer.accept")
        || type == QStringLiteral("transfer.reject")
        || type == QStringLiteral("transfer.chunk")
        || type == QStringLiteral("transfer.complete")
        || type == QStringLiteral("transfer.error");
}

// ---- 消息构建器 ----

QJsonObject Protocol::buildTextMessage(const QString& messageId,
                                       const QString& from,
                                       const QString& to,
                                       const QString& content)
{
    QJsonObject message;
    message.insert(QStringLiteral("type"), QStringLiteral("message.text"));
    message.insert(QStringLiteral("message_id"), messageId);
    message.insert(QStringLiteral("from"), from);
    message.insert(QStringLiteral("to"), to);
    message.insert(QStringLiteral("content"), content);
    message.insert(QStringLiteral("created_at"),
                   QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    return message;
}

QJsonObject Protocol::buildAck(const QString& messageId,
                               const QString& status)
{
    QJsonObject message;
    message.insert(QStringLiteral("type"), QStringLiteral("message.ack"));
    message.insert(QStringLiteral("message_id"), messageId);
    message.insert(QStringLiteral("status"), status);
    return message;
}

QJsonObject Protocol::buildSystemMessage(const QString& from,
                                         const QString& to,
                                         const QString& content)
{
    QJsonObject message;
    message.insert(QStringLiteral("type"), QStringLiteral("message.system"));
    message.insert(QStringLiteral("from"), from);
    message.insert(QStringLiteral("to"), to);
    message.insert(QStringLiteral("content"), content);
    message.insert(QStringLiteral("created_at"),
                   QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    return message;
}

// ---- 文件传输协议构建器 ----

QJsonObject Protocol::buildTransferRequest(const QString& transferId,
                                           const QString& from,
                                           const QString& to,
                                           const QString& fileName,
                                           qint64 fileSize,
                                           const QString& sha256,
                                           quint32 chunkSize,
                                           bool isFolder)
{
    QJsonObject message;
    message.insert(QStringLiteral("type"), QStringLiteral("transfer.request"));
    message.insert(QStringLiteral("transfer_id"), transferId);
    message.insert(QStringLiteral("from"), from);
    message.insert(QStringLiteral("to"), to);
    message.insert(QStringLiteral("file_name"), fileName);
    message.insert(QStringLiteral("file_size"), fileSize);
    message.insert(QStringLiteral("sha256"), sha256);
    message.insert(QStringLiteral("chunk_size"), static_cast<qint64>(chunkSize));
    message.insert(QStringLiteral("is_folder"), isFolder);
    return message;
}

QJsonObject Protocol::buildTransferAccept(const QString& transferId)
{
    QJsonObject message;
    message.insert(QStringLiteral("type"), QStringLiteral("transfer.accept"));
    message.insert(QStringLiteral("transfer_id"), transferId);
    return message;
}

QJsonObject Protocol::buildTransferReject(const QString& transferId,
                                          const QString& reason)
{
    QJsonObject message;
    message.insert(QStringLiteral("type"), QStringLiteral("transfer.reject"));
    message.insert(QStringLiteral("transfer_id"), transferId);
    if (!reason.isEmpty()) {
        message.insert(QStringLiteral("reason"), reason);
    }
    return message;
}

QJsonObject Protocol::buildTransferComplete(const QString& transferId,
                                            const QString& sha256)
{
    QJsonObject message;
    message.insert(QStringLiteral("type"), QStringLiteral("transfer.complete"));
    message.insert(QStringLiteral("transfer_id"), transferId);
    message.insert(QStringLiteral("sha256"), sha256);
    return message;
}

QJsonObject Protocol::buildTransferError(const QString& transferId,
                                         const QString& errorCode,
                                         const QString& errorMessage)
{
    QJsonObject message;
    message.insert(QStringLiteral("type"), QStringLiteral("transfer.error"));
    message.insert(QStringLiteral("transfer_id"), transferId);
    message.insert(QStringLiteral("error_code"), errorCode);
    message.insert(QStringLiteral("error_message"), errorMessage);
    return message;
}

// ---- 二进制 Chunk 帧序列化 ----

QByteArray Protocol::buildChunkFrame(const QString& transferId,
                                     quint32 chunkIndex,
                                     const QByteArray& data)
{
    const QByteArray idUtf8 = transferId.toUtf8();
    const quint32 idLength = static_cast<quint32>(idUtf8.size());
    const quint32 dataLength = static_cast<quint32>(data.size());

    // 计算总帧大小：4B_idLen + id + 4B_chunkIndex + 4B_dataLen + data
    const int totalSize = static_cast<int>(
        sizeof(quint32) + idUtf8.size() + sizeof(quint32) + sizeof(quint32) + data.size());

    QByteArray frame;
    frame.resize(totalSize);

    uchar* cursor = reinterpret_cast<uchar*>(frame.data());

    // 写入 transfer_id 长度和内容
    qToBigEndian(idLength, cursor);
    cursor += sizeof(quint32);
    std::memcpy(cursor, idUtf8.constData(), idUtf8.size());
    cursor += idUtf8.size();

    // 写入 chunk_index
    qToBigEndian(chunkIndex, cursor);
    cursor += sizeof(quint32);

    // 写入 data 长度和内容
    qToBigEndian(dataLength, cursor);
    cursor += sizeof(quint32);
    std::memcpy(cursor, data.constData(), data.size());

    return frame;
}

bool Protocol::parseChunkFrame(const QByteArray& frameData,
                               QString& transferIdOut,
                               quint32& chunkIndexOut,
                               QByteArray& dataOut,
                               QString& errorOut)
{
    const int minSize = static_cast<int>(sizeof(quint32) * 3);  // 至少 12 字节：3 个长度字段
    if (frameData.size() < minSize) {
        errorOut = QStringLiteral("Chunk frame too short: %1 bytes").arg(frameData.size());
        return false;
    }

    const uchar* cursor = reinterpret_cast<const uchar*>(frameData.constData());
    const uchar* end = cursor + frameData.size();

    // 读取 transfer_id 长度
    const quint32 idLength = qFromBigEndian<quint32>(cursor);
    cursor += sizeof(quint32);

    // 校验 transfer_id 长度合法性
    if (idLength == 0 || cursor + idLength > end) {
        errorOut = QStringLiteral("Invalid transfer_id length in chunk frame: %1").arg(idLength);
        return false;
    }
    transferIdOut = QString::fromUtf8(reinterpret_cast<const char*>(cursor),
                                      static_cast<int>(idLength));
    cursor += idLength;

    // 读取 chunk_index
    if (cursor + sizeof(quint32) > end) {
        errorOut = QStringLiteral("Chunk frame truncated at chunk_index");
        return false;
    }
    chunkIndexOut = qFromBigEndian<quint32>(cursor);
    cursor += sizeof(quint32);

    // 读取 data 长度
    if (cursor + sizeof(quint32) > end) {
        errorOut = QStringLiteral("Chunk frame truncated at data length");
        return false;
    }
    const quint32 dataLength = qFromBigEndian<quint32>(cursor);
    cursor += sizeof(quint32);

    // 校验 data 长度
    if (cursor + dataLength > end) {
        errorOut = QStringLiteral("Chunk frame data truncated: expected %1 bytes, got %2")
                       .arg(dataLength)
                       .arg(end - cursor);
        return false;
    }
    dataOut = QByteArray(reinterpret_cast<const char*>(cursor),
                         static_cast<int>(dataLength));

    return true;
}

bool Protocol::isLikelyChunkFrame(const QByteArray& frameData)
{
    // JSON 帧以 '{' (0x7B) 开头；chunk 二进制帧首字节是 transfer_id 长度的 BE 高位部分，
    // 长度通常远小于 0x7B（12 字符 UUID 对应 0x00），因此首字节非 '{' 即可初步判定。
    return !frameData.isEmpty() && frameData.at(0) != '{';
}

} // namespace FengSui
