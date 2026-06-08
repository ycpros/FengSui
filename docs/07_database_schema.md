# 07 — 数据库 Schema (SQLite)

> 数据库文件：`{AppData}/FengSui/fengsui.db`
> 使用 QSqlDatabase + QSqlQuery，封装在 `storage/` 层。

---

## 版本管理

```sql
CREATE TABLE IF NOT EXISTS schema_version (
    version INTEGER PRIMARY KEY,
    applied_at TEXT NOT NULL DEFAULT (datetime('now'))
);
```

V1.0 初始 schema，version = 1。

---

## 表定义

### peers — 设备/联系人

```sql
CREATE TABLE peers (
    peer_id         TEXT PRIMARY KEY,
    display_name    TEXT NOT NULL,
    device_name     TEXT NOT NULL,
    ip              TEXT,
    port            INTEGER DEFAULT 8787,
    os              TEXT,
    online          INTEGER DEFAULT 0,
    last_seen_at    TEXT,
    share_enabled   INTEGER DEFAULT 0,
    version         TEXT
);
CREATE INDEX idx_peers_online ON peers(online);
```

### conversations — 会话

```sql
CREATE TABLE conversations (
    conversation_id TEXT PRIMARY KEY,
    peer_id         TEXT NOT NULL,
    last_message    TEXT,
    last_message_at TEXT,
    unread_count    INTEGER DEFAULT 0,
    FOREIGN KEY (peer_id) REFERENCES peers(peer_id)
);
CREATE INDEX idx_conversations_peer ON conversations(peer_id);
CREATE INDEX idx_conversations_time ON conversations(last_message_at DESC);
```

### messages — 消息

```sql
CREATE TABLE messages (
    message_id      TEXT PRIMARY KEY,
    conversation_id TEXT NOT NULL,
    sender_id       TEXT NOT NULL,
    type            TEXT NOT NULL DEFAULT 'text',  -- 'text' / 'file_request' / 'system'
    content         TEXT,
    file_name       TEXT,
    file_size       INTEGER DEFAULT 0,
    transfer_id     TEXT,
    status          TEXT NOT NULL DEFAULT 'sending', -- 'sending'/'sent'/'delivered'/'failed'
    created_at      TEXT NOT NULL,
    FOREIGN KEY (conversation_id) REFERENCES conversations(conversation_id)
);
CREATE INDEX idx_messages_conv ON messages(conversation_id, created_at);
CREATE INDEX idx_messages_transfer ON messages(transfer_id);
```

### transfer_tasks — 文件传输任务

```sql
CREATE TABLE transfer_tasks (
    transfer_id      TEXT PRIMARY KEY,
    direction        TEXT NOT NULL,  -- 'upload' / 'download'
    peer_id          TEXT NOT NULL,
    file_name        TEXT NOT NULL,
    file_path        TEXT,
    file_size        INTEGER NOT NULL,
    transferred_bytes INTEGER DEFAULT 0,
    sha256           TEXT,
    status           TEXT NOT NULL DEFAULT 'waiting',
    error_message    TEXT,
    created_at       TEXT NOT NULL,
    completed_at     TEXT
);
CREATE INDEX idx_transfers_status ON transfer_tasks(status);
CREATE INDEX idx_transfers_peer ON transfer_tasks(peer_id);
```

### shared_folders — 共享目录

```sql
CREATE TABLE shared_folders (
    share_id     TEXT PRIMARY KEY,
    local_path   TEXT NOT NULL,
    display_name TEXT NOT NULL,
    is_active    INTEGER DEFAULT 1
);
```

### access_grants — 共享访问授权

```sql
CREATE TABLE access_grants (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    peer_id    TEXT NOT NULL,
    share_id   TEXT NOT NULL,
    granted_at TEXT NOT NULL,
    remember   INTEGER DEFAULT 0,
    FOREIGN KEY (share_id) REFERENCES shared_folders(share_id),
    UNIQUE(peer_id, share_id)
);
```

### download_logs — 下载记录

```sql
CREATE TABLE download_logs (
    log_id        TEXT PRIMARY KEY,
    peer_id       TEXT NOT NULL,
    share_id      TEXT,
    remote_path   TEXT,
    local_path    TEXT,
    file_name     TEXT NOT NULL,
    file_size     INTEGER,
    downloaded_at TEXT NOT NULL,
    success       INTEGER DEFAULT 1
);
```

### settings — 应用设置

```sql
CREATE TABLE settings (
    key   TEXT PRIMARY KEY,
    value TEXT
);
```

内置 key：
- `display_name` — 用户昵称
- `download_dir` — 默认下载目录
- `discovery_enabled` — "1" / "0"
- `listen_port` — TCP 端口
- `onboarding_completed` — "1" / "0"
- `auto_start` — "1" / "0"
- `minimize_to_tray` — "1" / "0"

---

## 关键查询示例

```sql
-- 获取最近会话列表（按最后消息时间排序）
SELECT c.*, p.display_name, p.online
FROM conversations c
JOIN peers p ON c.peer_id = p.peer_id
ORDER BY c.last_message_at DESC;

-- 获取某个会话的消息历史（分页，每页 50 条）
SELECT * FROM messages
WHERE conversation_id = ?
ORDER BY created_at DESC
LIMIT 50 OFFSET ?;

-- 获取进行中的传输任务
SELECT * FROM transfer_tasks WHERE status IN ('waiting', 'transferring');

-- 统计未读消息总数
SELECT SUM(unread_count) FROM conversations;
```

## 迁移策略

V1.0 使用硬编码 schema version = 1。后续版本增加 `Migration` 类，按 version 序号逐级迁移（`ALTER TABLE` 或重建）。首次启动时自动执行迁移。
