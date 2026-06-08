// Database.h
// SQLite 数据库管理：初始化连接、执行 schema 迁移、提供 QSqlDatabase 实例。
// 数据库文件保存在 {AppData}/FengSui/fengsui.db。

#pragma once

#include <QObject>
#include <QSqlDatabase>

namespace FengSui {

// SQLite 数据库管理器。
// 应用启动时调用 initialize() 完成建库建表，各 Repository 通过 database() 获取连接。
class Database : public QObject {
    Q_OBJECT

public:
    explicit Database(QObject* parent = nullptr);

    // 使用指定 SQLite 文件路径创建数据库管理器。
    // databasePath: 绝对文件路径，主要供自动化测试隔离数据库使用；为空时退回默认 AppData 路径。
    explicit Database(const QString& databasePath, QObject* parent = nullptr);

    ~Database() override;

    // 初始化数据库：打开/创建 SQLite 文件，执行 schema 迁移。
    // 返回 true 表示初始化成功。
    bool initialize();

    // 获取数据库连接实例，供 Repository 层使用
    QSqlDatabase database() const;

    // 获取数据库文件路径
    QString databasePath() const;

private:
    // 执行建表语句（按 07_database_schema.md 定义）
    bool createTables();

    // 插入初始 schema 版本记录
    bool initializeVersion();

    QString m_connectionName;  // QSqlDatabase 连接名（唯一）
    QString m_databasePathOverride;  // 测试或特殊启动场景指定的数据库文件路径
};

} // namespace FengSui
