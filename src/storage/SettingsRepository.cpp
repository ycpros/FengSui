// SettingsRepository.cpp
// SQLite settings 表访问实现。

#include "storage/SettingsRepository.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "storage/Database.h"

namespace FengSui {

SettingsRepository::SettingsRepository(Database* database, QObject* parent)
    : QObject(parent)
    , m_database(database)
{
}

QString SettingsRepository::value(const QString& key, const QString& defaultValue) const
{
    if (!m_database) {
        qWarning() << "SettingsRepository has no database, returning default for key:" << key;
        return defaultValue;
    }

    QSqlQuery query(m_database->database());
    query.prepare("SELECT value FROM settings WHERE key = ?");
    query.addBindValue(key);

    if (!query.exec()) {
        qWarning() << "Failed to read setting:" << key << query.lastError().text();
        return defaultValue;
    }

    if (!query.next()) {
        return defaultValue;
    }

    return query.value(0).toString();
}

bool SettingsRepository::setValue(const QString& key, const QString& value)
{
    if (!m_database) {
        qWarning() << "SettingsRepository has no database, cannot write key:" << key;
        return false;
    }

    QSqlQuery query(m_database->database());
    query.prepare("INSERT OR REPLACE INTO settings (key, value) VALUES (?, ?)");
    query.addBindValue(key);
    query.addBindValue(value);

    if (!query.exec()) {
        qWarning() << "Failed to write setting:" << key << query.lastError().text();
        return false;
    }

    return true;
}

bool SettingsRepository::contains(const QString& key) const
{
    if (!m_database) {
        qWarning() << "SettingsRepository has no database, cannot check key:" << key;
        return false;
    }

    QSqlQuery query(m_database->database());
    query.prepare("SELECT 1 FROM settings WHERE key = ? LIMIT 1");
    query.addBindValue(key);

    if (!query.exec()) {
        qWarning() << "Failed to check setting:" << key << query.lastError().text();
        return false;
    }

    return query.next();
}

} // namespace FengSui
