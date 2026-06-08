# 06 — 通信协议

> 所有协议消息使用 JSON 格式，UTF-8 编码。UDP 用于发现，TCP 用于消息和文件传输。

---

## 1. UDP 发现协议

UDP 端口：`8788`（多播地址 `239.255.100.100`，TTL=2）

### 1.1 presence.hello — 上线广播

```json
{
  "type": "presence.hello",
  "peer_id": "a1b2c3d4",
  "display_name": "Lily-PC",
  "device_name": "DESKTOP-001",
  "ip": "192.168.1.20",
  "tcp_port": 8787,
  "os": "windows",
  "share_enabled": true,
  "version": "0.1.0"
}
```

| 字段 | 类型 | 说明 |
|---|---|---|
| type | string | `"presence.hello"` |
| peer_id | string | 设备唯一标识 |
| display_name | string | 用户昵称 |
| device_name | string | 系统主机名 |
| ip | string | 设备 IP 地址 |
| tcp_port | int | TCP 消息服务端口 |
| os | string | `"windows"` / `"linux"` / `"macos"` |
| share_enabled | bool | 是否启用共享 |
| version | string | 客户端版本 |

### 1.2 presence.heartbeat — 心跳

每 5 秒发送一次，超时 15 秒未收到则判定离线。

```json
{
  "type": "presence.heartbeat",
  "peer_id": "a1b2c3d4"
}
```

### 1.3 presence.goodbye — 主动离线

客户端退出前发送（尽力而为，UDP 可能丢包）。

```json
{
  "type": "presence.goodbye",
  "peer_id": "a1b2c3d4"
}
```

---

## 2. TCP 消息协议

TCP 监听端口：默认 `8787`。
消息格式：`[4字节大端长度][JSON 正文]`。

### 2.1 message.text — 文本消息

```json
{
  "type": "message.text",
  "message_id": "msg_a1b2c3d4",
  "from": "a1b2c3d4",
  "to": "e5f6g7h8",
  "content": "你好，文件我发你了。",
  "created_at": "2026-06-04T15:30:00+08:00"
}
```

### 2.2 message.ack — 消息确认

```json
{
  "type": "message.ack",
  "message_id": "msg_a1b2c3d4",
  "status": "delivered"
}
```

| status | 说明 |
|---|---|
| delivered | 已送达 |
| failed | 接收失败 |

### 2.3 message.system — 系统消息

```json
{
  "type": "message.system",
  "from": "a1b2c3d4",
  "to": "e5f6g7h8",
  "content": "对方已接收文件 design.psd",
  "created_at": "2026-06-04T15:30:05+08:00"
}
```

---

## 3. 文件传输协议

文件传输复用 TCP 消息通道。大文件分块传输：每块 8MB（`chunk_size` 可协商）。

### 3.1 transfer.request — 传输请求

```json
{
  "type": "transfer.request",
  "transfer_id": "tr_a1b2c3d4",
  "from": "a1b2c3d4",
  "to": "e5f6g7h8",
  "file_name": "design.psd",
  "file_size": 845928734,
  "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
  "chunk_size": 8388608,
  "is_folder": false
}
```

### 3.2 transfer.accept — 接受

```json
{
  "type": "transfer.accept",
  "transfer_id": "tr_a1b2c3d4"
}
```

### 3.3 transfer.reject — 拒绝

```json
{
  "type": "transfer.reject",
  "transfer_id": "tr_a1b2c3d4",
  "reason": "文件太大"
}
```

### 3.4 transfer.chunk — 数据块

二进制格式：`[4字节大端 transfer_id 长度][transfer_id UTF-8][4字节大端 chunk_index][4字节大端 data 长度][data 二进制]`

### 3.5 transfer.complete — 传输完成

```json
{
  "type": "transfer.complete",
  "transfer_id": "tr_a1b2c3d4",
  "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
}
```

### 3.6 transfer.error — 传输错误

```json
{
  "type": "transfer.error",
  "transfer_id": "tr_a1b2c3d4",
  "error_code": "NETWORK_TIMEOUT",
  "error_message": "连接超时"
}
```

---

## 4. 共享目录协议

通过 TCP 请求 HTTP 风格 API，响应 JSON。

### 4.1 share.list — 获取共享源列表

请求：`GET /share/list`

```json
{
  "type": "share.list",
  "from": "a1b2c3d4"
}
```

响应：
```json
{
  "type": "share.list.reply",
  "shares": [
    {
      "share_id": "sh_001",
      "display_name": "设计稿",
      "file_count": 42
    }
  ]
}
```

### 4.2 share.items — 浏览目录

请求：`GET /share/{share_id}/items?path=/2026/prototype`

响应：
```json
{
  "type": "share.items.reply",
  "share_id": "sh_001",
  "path": "/2026/prototype",
  "items": [
    {
      "name": "design.psd",
      "size": 845928734,
      "modified_at": "2026-06-01T10:00:00",
      "is_dir": false
    }
  ]
}
```

### 4.3 share.download — 下载文件

请求：`GET /share/{share_id}/download?path=/2026/prototype/design.psd`

响应：二进制文件流（先回 JSON 头，再回文件数据）
```json
{
  "type": "share.download.reply",
  "file_name": "design.psd",
  "file_size": 845928734,
  "sha256": "e3b0..."
}
```

---

## 5. 错误码

| 错误码 | 说明 |
|---|---|
| NETWORK_TIMEOUT | 网络连接超时 |
| CONNECTION_REFUSED | 对方拒绝连接 |
| FILE_NOT_FOUND | 请求的文件不存在 |
| ACCESS_DENIED | 访问被拒绝 |
| INVALID_MESSAGE | 消息格式错误 |
| DISK_FULL | 磁盘空间不足 |
| TRANSFER_CANCELLED | 传输被取消 |
| UNKNOWN | 未知错误 |
