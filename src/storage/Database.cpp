// Database.cpp
// SQLite 数据库初始化与 schema 创建实现。

#include "storage/Database.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>

namespace FengSui {

Database::Database(QObject* parent)
    : QObject(parent)
{
    // 生成唯一连接名，避免多实例冲突
    m_connectionName = "fengsui_db_" + QUuid::createUuid().toString(QUuid::Id128);
}

Database::Database(const QString& databasePath, QObject* parent)
    : QObject(parent)
    , m_databasePathOverride(databasePath)
{
    // 生成唯一连接名，避免测试中多数据库实例互相复用连接。
    m_connectionName = "fengsui_db_" + QUuid::createUuid().toString(QUuid::Id128);
}

Database::~Database()
{
    // 关闭并移除数据库连接
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase::database(m_connectionName).close();
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool Database::initialize()
{
    // 确保数据目录存在
    QString dataDir = QFileInfo(databasePath()).absolutePath();
    if (dataDir.isEmpty()) {
        dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }
    QDir().mkpath(dataDir);

    // 创建 SQLite 连接
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(databasePath());

    if (!db.open()) {
        qCritical() << "Failed to open database:" << db.lastError().text();
        return false;
    }

    // 启用 WAL 模式，提升并发读写性能
    QSqlQuery pragma(db);
    pragma.exec("PRAGMA journal_mode=WAL");
    pragma.exec("PRAGMA foreign_keys=ON");

    qInfo() << "Database opened:" << databasePath();

    // 执行建表
    if (!createTables()) {
        qCritical() << "Failed to create database tables";
        return false;
    }

    // 初始化版本号
    if (!initializeVersion()) {
        qCritical() << "Failed to initialize schema version";
        return false;
    }

    qInfo() << "Database initialized successfully, schema version 1";
    return true;
}

QSqlDatabase Database::database() const
{
    return QSqlDatabase::database(m_connectionName);
}

QString Database::databasePath() const
{
    if (!m_databasePathOverride.trimmed().isEmpty()) {
        return m_databasePathOverride;
    }

    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dataDir + "/fengsui.db";
}

bool Database::createTables()
{
    QSqlDatabase db = database();

    // 按 07_database_schema.md 定义的完整 schema
    const QStringList statements = {
        // schema 版本管理
        R"(
            CREATE TABLE IF NOT EXISTS schema_version (
                version     INTEGER PRIMARY KEY,
                applied_at  TEXT NOT NULL DEFAULT (datetime('now'))
            )
        )",

        // 设备/联系人
        R"(
            CREATE TABLE IF NOT EXISTS peers (
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
            )
        )",

        // 会话
        R"(
            CREATE TABLE IF NOT EXISTS conversations (
                conversation_id TEXT PRIMARY KEY,
                peer_id         TEXT NOT NULL,
                last_message    TEXT,
                last_message_at TEXT,
                unread_count    INTEGER DEFAULT 0,
                FOREIGN KEY (peer_id) REFERENCES peers(peer_id)
            )
        )",

        // 消息
        R"(
            CREATE TABLE IF NOT EXISTS messages (
                message_id      TEXT PRIMARY KEY,
                conversation_id TEXT NOT NULL,
                sender_id       TEXT NOT NULL,
                type            TEXT NOT NULL DEFAULT 'text',
                content         TEXT,
                file_name       TEXT,
                file_size       INTEGER DEFAULT 0,
                transfer_id     TEXT,
                status          TEXT NOT NULL DEFAULT 'sending',
                created_at      TEXT NOT NULL,
                FOREIGN KEY (conversation_id) REFERENCES conversations(conversation_id)
            )
        )",

        // 文件传输任务
        R"(
            CREATE TABLE IF NOT EXISTS transfer_tasks (
                transfer_id       TEXT PRIMARY KEY,
                direction         TEXT NOT NULL,
                peer_id           TEXT NOT NULL,
                file_name         TEXT NOT NULL,
                file_path         TEXT,
                file_size         INTEGER NOT NULL,
                transferred_bytes INTEGER DEFAULT 0,
                sha256            TEXT,
                status            TEXT NOT NULL DEFAULT 'waiting',
                error_message     TEXT,
                created_at        TEXT NOT NULL,
                completed_at      TEXT
            )
        )",

        // 共享目录
        R"(
            CREATE TABLE IF NOT EXISTS shared_folders (
                share_id     TEXT PRIMARY KEY,
                local_path   TEXT NOT NULL,
                display_name TEXT NOT NULL,
                is_active    INTEGER DEFAULT 1
            )
        )",

        // 共享访问授权
        R"(
            CREATE TABLE IF NOT EXISTS access_grants (
                id          INTEGER PRIMARY KEY AUTOINCREMENT,
                peer_id     TEXT NOT NULL,
                share_id    TEXT NOT NULL,
                granted_at  TEXT NOT NULL,
                remember    INTEGER DEFAULT 0,
                FOREIGN KEY (share_id) REFERENCES shared_folders(share_id),
                UNIQUE(peer_id, share_id)
            )
        )",

        // 下载记录
        R"(
            CREATE TABLE IF NOT EXISTS download_logs (
                log_id        TEXT PRIMARY KEY,
                peer_id       TEXT NOT NULL,
                share_id      TEXT,
                remote_path   TEXT,
                local_path    TEXT,
                file_name     TEXT NOT NULL,
                file_size     INTEGER,
                downloaded_at TEXT NOT NULL,
                success       INTEGER DEFAULT 1
            )
        )",

        // 应用设置（key-value 表）
        R"(
            CREATE TABLE IF NOT EXISTS settings (
                key   TEXT PRIMARY KEY,
                value TEXT
            )
        )",

        // 手动添加设备历史
        R"(
            CREATE TABLE IF NOT EXISTS manual_peers (
                name            TEXT NOT NULL,
                host            TEXT NOT NULL,
                port            INTEGER NOT NULL,
                last_success_at TEXT NOT NULL,
                UNIQUE(host, port)
            )
        )",
    };

    QSqlQuery query(db);
    for (const QString& sql : statements) {
        if (!query.exec(sql)) {
            qCritical() << "Failed to create table:" << query.lastError().text();
            qCritical() << "SQL:" << sql;
            return false;
        }
    }

    // 创建索引
    const QStringList indexes = {
        "CREATE INDEX IF NOT EXISTS idx_peers_online ON peers(online)",
        "CREATE INDEX IF NOT EXISTS idx_conversations_peer ON conversations(peer_id)",
        "CREATE INDEX IF NOT EXISTS idx_conversations_time ON conversations(last_message_at DESC)",
        "CREATE INDEX IF NOT EXISTS idx_messages_conv ON messages(conversation_id, created_at)",
        "CREATE INDEX IF NOT EXISTS idx_messages_transfer ON messages(transfer_id)",
        "CREATE INDEX IF NOT EXISTS idx_transfers_status ON transfer_tasks(status)",
        "CREATE INDEX IF NOT EXISTS idx_transfers_peer ON transfer_tasks(peer_id)",
        "CREATE INDEX IF NOT EXISTS idx_manual_peers_time ON manual_peers(last_success_at DESC)",
    };

    for (const QString& sql : indexes) {
        if (!query.exec(sql)) {
            qCritical() << "Failed to create index:" << query.lastError().text();
            return false;
        }
    }

    return true;
}

bool Database::initializeVersion()
{
    QSqlQuery query(database());
    query.prepare("SELECT COUNT(*) FROM schema_version");
    if (!query.exec() || !query.next()) {
        qCritical() << "Failed to query schema_version";
        return false;
    }

    // 表为空则插入初始版本记录
    if (query.value(0).toInt() == 0) {
        QSqlQuery insert(database());
        insert.prepare("INSERT INTO schema_version (version) VALUES (1)");
        if (!insert.exec()) {
            qCritical() << "Failed to insert schema_version";
            return false;
        }
    }

    return true;
}

} // namespace FengSui
