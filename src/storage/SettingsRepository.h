// SettingsRepository.h
// SQLite settings 表访问封装。应用设置统一通过此 Repository 读写。

#pragma once

#include <QObject>
#include <QString>

namespace FengSui {

class Database;

// 设置持久化仓库。
// 只访问 settings(key, value) 表，对外提供简单 key-value 接口。
class SettingsRepository : public QObject {
    Q_OBJECT

public:
    explicit SettingsRepository(Database* database, QObject* parent = nullptr);

    // 读取设置值；记录不存在或查询失败时返回 defaultValue。
    QString value(const QString& key, const QString& defaultValue = QString()) const;

    // 写入设置值；成功返回 true，失败返回 false。
    bool setValue(const QString& key, const QString& value);

    // 判断设置 key 是否存在。
    bool contains(const QString& key) const;

private:
    Database* m_database = nullptr;
};

} // namespace FengSui
