// TcpConnection.cpp
// TCP 连接封装实现：socket 生命周期、读缓冲帧提取、消息收发。
// 支持 JSON 控制帧和二进制 chunk 帧共存于同一 TCP 连接。

#include "network/TcpConnection.h"
#include "network/Protocol.h"

#include <QAbstractSocket>
#include <QDebug>
#include <QTcpSocket>
#include <QtEndian>

namespace FengSui {

namespace {

// chunk 帧中 transfer_id 的最大字节数（UUID 编码后 ~40 字节，256 已足够）
constexpr quint32 kMaxTransferIdLength = 256;

// chunk 帧中单块数据最大字节数（与 06_protocol.md 定义的 chunk_size 上限一致）
constexpr quint32 kMaxChunkDataLength = 8 * 1024 * 1024;  // 8 MB

} // namespace

TcpConnection::TcpConnection(QObject* parent)
    : QObject(parent)
{
}

TcpConnection::~TcpConnection()
{
    if (m_socket) {
        QTcpSocket* socket = m_socket;
        if (m_ownsSocket) {
            // 析构阶段必须同步释放 socket，避免父对象销毁和 DeferredDelete 交错造成悬空访问。
            socket->disconnect(this);
            socket->abort();
            socket->setParent(nullptr);
            delete socket;
        } else {
            socket->disconnect(this);
        }
        m_socket = nullptr;
    }
}

void TcpConnection::setSocket(QTcpSocket* socket)
{
    if (!socket) {
        qWarning() << "TcpConnection::setSocket called with null socket";
        return;
    }

    m_socket = socket;
    // 入站 socket 原本由 QTcpServer 创建；这里转交给 TcpConnection 管理，
    // 确保连接封装和底层 socket 生命周期一致。
    m_socket->setParent(this);
    m_ownsSocket = true;

    // 连接 socket 信号，开始监听数据
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpConnection::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpConnection::onSocketDisconnected);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &TcpConnection::onSocketError);

    qInfo() << "TcpConnection accepted from" << m_socket->peerAddress().toString()
            << m_socket->peerPort();
}

void TcpConnection::connectToHost(const QString& host, quint16 port)
{
    if (m_socket) {
        qWarning() << "TcpConnection already has a socket, disconnecting first";
        disconnect();
    }

    m_socket = new QTcpSocket(this);
    m_ownsSocket = true;

    connect(m_socket, &QTcpSocket::connected, this, &TcpConnection::onSocketConnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpConnection::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpConnection::onSocketDisconnected);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &TcpConnection::onSocketError);

    qInfo() << "TcpConnection connecting to" << host << port;
    m_socket->connectToHost(host, port);
}

bool TcpConnection::sendMessage(const QJsonObject& message)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "Cannot send message: socket is not connected";
        return false;
    }

    const QByteArray frame = Protocol::pack(message);
    if (frame.isEmpty()) {
        qWarning() << "Failed to pack message for sending";
        return false;
    }

    const qint64 bytesWritten = m_socket->write(frame);
    if (bytesWritten != frame.size()) {
        qWarning() << "Short write on TcpConnection: wrote"
                   << bytesWritten << "of" << frame.size() << "bytes";
        return false;
    }

    return true;
}

bool TcpConnection::sendBinaryChunk(const QByteArray& chunkFrame)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "Cannot send chunk: socket is not connected";
        return false;
    }

    if (chunkFrame.isEmpty()) {
        qWarning() << "Cannot send empty chunk frame";
        return false;
    }

    // chunk 帧由 Protocol::buildChunkFrame 预制好了完整 wire format，
    // 直接写入 socket 即可，不需再经过 pack()。
    const qint64 bytesWritten = m_socket->write(chunkFrame);
    if (bytesWritten != chunkFrame.size()) {
        qWarning() << "Short write on binary chunk: wrote"
                   << bytesWritten << "of" << chunkFrame.size() << "bytes";
        return false;
    }

    return true;
}

QString TcpConnection::peerId() const
{
    return m_peerId;
}

void TcpConnection::setPeerId(const QString& peerId)
{
    m_peerId = peerId;
}

bool TcpConnection::isConnected() const
{
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

void TcpConnection::disconnect()
{
    if (m_socket) {
        m_socket->disconnectFromHost();
    }
}

void TcpConnection::onReadyRead()
{
    if (!m_socket) {
        return;
    }

    // 将新数据追加到读缓冲
    m_readBuffer.append(m_socket->readAll());

    processReadBuffer();
}

void TcpConnection::onSocketConnected()
{
    qInfo() << "TcpConnection established to" << m_socket->peerAddress().toString()
            << m_socket->peerPort();
    emit connected();
}

void TcpConnection::onSocketDisconnected()
{
    qInfo() << "TcpConnection disconnected, peer:" << m_peerId;
    emit disconnected();
}

void TcpConnection::onSocketError()
{
    if (m_socket) {
        const QString error = m_socket->errorString();
        qWarning() << "TcpConnection socket error:" << error;
        emit errorOccurred(error);
    }
}

// 尝试从读缓冲中提取一个 JSON 帧（长度前缀格式）。
// 成功返回 true；数据不完整返回 false（errorOut 为空）；格式错误返回 false（errorOut 写入原因）。
static bool tryExtractJsonFrame(QByteArray& readBuffer, QJsonObject& messageOut,
                                QString& errorOut)
{
    QByteArray frame;
    if (!Protocol::extractFrame(readBuffer, frame, errorOut)) {
        return false;  // 数据不完整或已 emit 错误
    }

    return Protocol::unpack(frame, messageOut, errorOut);
}

void TcpConnection::processReadBuffer()
{
    // 循环处理缓冲，可能混合 JSON 帧与 chunk 帧
    while (m_readBuffer.size() >= static_cast<int>(sizeof(quint32))) {

        // 需要至少 5 字节才能区分帧类型：4 字节长度前缀 + 1 字节载荷首字符
        if (m_readBuffer.size() < static_cast<int>(sizeof(quint32)) + 1) {
            return;  // 等待更多数据
        }

        // 读取首个长度字段（JSON 帧为 JSON 长度，chunk 帧为 transfer_id 长度）
        const quint32 firstLength = qFromBigEndian<quint32>(
            reinterpret_cast<const uchar*>(m_readBuffer.constData()));

        // 检查载荷首字节是否为 JSON 起始符 '{'
        const char firstPayloadByte = m_readBuffer.at(static_cast<int>(sizeof(quint32)));

        if (firstPayloadByte == '{') {
            // ---- JSON 帧路径 ----
            QJsonObject message;
            QString error;
            if (tryExtractJsonFrame(m_readBuffer, message, error)) {
                emit messageReceived(message);
            } else if (!error.isEmpty()) {
                // JSON 解析错误，丢弃当前帧并继续
                qWarning() << "JSON frame error:" << error;
                emit errorOccurred(QStringLiteral("JSON frame: %1").arg(error));
                if (m_readBuffer.isEmpty()) {
                    return;
                }
            } else {
                // 数据不完整，等待更多数据
                return;
            }
        } else {
            // ---- 二进制 chunk 帧路径 ----
            // chunk 帧格式：[4B idLen][id UTF-8][4B chunkIdx][4B dataLen][data]

            // 校验 transfer_id 长度
            if (firstLength == 0 || firstLength > kMaxTransferIdLength) {
                qWarning() << "Invalid transfer_id length in chunk frame:" << firstLength;
                emit errorOccurred(QStringLiteral("Chunk frame: invalid transfer_id length %1")
                                       .arg(firstLength));
                m_readBuffer.clear();  // 无法恢复同步，清空缓冲
                return;
            }

            // 计算报头总字节数：4B_idLen + id + 4B_chunkIdx + 4B_dataLen
            const int headerSize = static_cast<int>(
                sizeof(quint32) + firstLength + sizeof(quint32) + sizeof(quint32));

            if (m_readBuffer.size() < headerSize) {
                return;  // 报头不完整，等待更多数据
            }

            // 读取 chunk_index 和 data 长度
            const uchar* headerCursor = reinterpret_cast<const uchar*>(
                m_readBuffer.constData()) + sizeof(quint32) + firstLength;
            const quint32 chunkIndex = qFromBigEndian<quint32>(headerCursor);
            headerCursor += sizeof(quint32);
            const quint32 dataLength = qFromBigEndian<quint32>(headerCursor);

            // 校验 data 长度
            if (dataLength > kMaxChunkDataLength) {
                qWarning() << "Chunk data length exceeds maximum:" << dataLength;
                emit errorOccurred(QStringLiteral("Chunk frame: data length %1 exceeds max")
                                       .arg(dataLength));
                m_readBuffer.clear();
                return;
            }

            const int totalFrameSize = headerSize + static_cast<int>(dataLength);
            if (m_readBuffer.size() < totalFrameSize) {
                return;  // 数据载荷不完整，等待更多数据
            }

            // 提取完整 chunk 帧
            const QByteArray chunkFrame = m_readBuffer.left(totalFrameSize);
            m_readBuffer.remove(0, totalFrameSize);

            // 解析 chunk 帧各字段
            QString transferId;
            quint32 parsedChunkIndex;
            QByteArray data;
            QString error;
            if (Protocol::parseChunkFrame(chunkFrame, transferId, parsedChunkIndex,
                                          data, error)) {
                emit binaryChunkReceived(transferId, parsedChunkIndex, data);
            } else {
                qWarning() << "Chunk frame parse error:" << error;
                emit errorOccurred(QStringLiteral("Chunk frame: %1").arg(error));
            }
        }
    }
}

} // namespace FengSui
