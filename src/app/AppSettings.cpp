// AppSettings.cpp
// 应用设置实现：基于 SQLite settings 表持久化读写用户配置。

#include "app/AppSettings.h"

#include <QDebug>
#include <QHostInfo>
#include <QStandardPaths>
#include <QUuid>

#include "storage/SettingsRepository.h"

namespace FengSui {

// settings 表 key 名称
static const char* KEY_DISPLAY_NAME        = "display_name";
static const char* KEY_DOWNLOAD_DIR        = "download_dir";
static const char* KEY_AUTO_START          = "auto_start";
static const char* KEY_MINIMIZE_TO_TRAY    = "minimize_to_tray";
static const char* KEY_THEME_MODE          = "theme_mode";
static const char* KEY_PEER_ID             = "peer_id";
static const char* KEY_DISCOVERY_ENABLED   = "discovery_enabled";
static const char* KEY_LISTEN_PORT         = "listen_port";
static const char* KEY_ONBOARDING_COMPLETED = "onboarding_completed";
static const char* KEY_SELECTED_INTERFACES  = "selected_interfaces";
static const char* KEY_ALLOWED_CIDRS        = "allowed_cidrs";
static const char* KEY_NETWORK_MODE         = "network_mode";

AppSettings::AppSettings(SettingsRepository* repository, QObject* parent)
    : QObject(parent)
    , m_repository(repository)
{
}

bool AppSettings::load()
{
    if (!m_repository) {
        return false;
    }

    // peer_id 是发现协议里的稳定身份，首次运行时生成并写入 settings 表。
    QString currentPeerId = m_repository->value(KEY_PEER_ID).trimmed();
    if (currentPeerId.isEmpty()) {
        // 使用随机 UUID 避免同一局域网内主机名重复导致身份冲突。
        currentPeerId = "peer_" + QUuid::createUuid().toString(QUuid::Id128);
        if (!m_repository->setValue(KEY_PEER_ID, currentPeerId)) {
            qWarning() << "Failed to initialize peer_id";
            return false;
        }
    }

    return true;
}

void AppSettings::save()
{
    // 设置项在 setter 中即时写入 SQLite，无需额外 flush。
}

// ---- 常规设置 ----

QString AppSettings::displayName() const
{
    return readValue(KEY_DISPLAY_NAME, QHostInfo::localHostName());
}

void AppSettings::setDisplayName(const QString& name)
{
    writeValue(KEY_DISPLAY_NAME, name);
}

QString AppSettings::downloadDir() const
{
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    return readValue(KEY_DOWNLOAD_DIR, defaultDir);
}

void AppSettings::setDownloadDir(const QString& dir)
{
    writeValue(KEY_DOWNLOAD_DIR, dir);
}

bool AppSettings::autoStart() const
{
    return readValue(KEY_AUTO_START, "0") == "1";
}

void AppSettings::setAutoStart(bool enabled)
{
    writeValue(KEY_AUTO_START, enabled ? "1" : "0");
}

bool AppSettings::minimizeToTray() const
{
    return readValue(KEY_MINIMIZE_TO_TRAY, "1") == "1";
}

void AppSettings::setMinimizeToTray(bool enabled)
{
    writeValue(KEY_MINIMIZE_TO_TRAY, enabled ? "1" : "0");
}

QString AppSettings::themeMode() const
{
    return readValue(KEY_THEME_MODE, QStringLiteral("system"));
}

void AppSettings::setThemeMode(const QString& mode)
{
    writeValue(KEY_THEME_MODE, mode.trimmed());
}

// ---- 网络设置 ----

QString AppSettings::peerId() const
{
    // peer_id 必须由 load() 初始化；这里不再临时生成，避免调用点得到非持久身份。
    return readValue(KEY_PEER_ID);
}

void AppSettings::setPeerId(const QString& peerId)
{
    writeValue(KEY_PEER_ID, peerId.trimmed());
}

bool AppSettings::discoveryEnabled() const
{
    return readValue(KEY_DISCOVERY_ENABLED, "1") == "1";
}

void AppSettings::setDiscoveryEnabled(bool enabled)
{
    writeValue(KEY_DISCOVERY_ENABLED, enabled ? "1" : "0");
}

quint16 AppSettings::listenPort() const
{
    return static_cast<quint16>(readValue(KEY_LISTEN_PORT, "8787").toUInt());
}

void AppSettings::setListenPort(quint16 port)
{
    writeValue(KEY_LISTEN_PORT, QString::number(port));
}

// ---- 网络策略 ----

QString AppSettings::selectedInterfaces() const
{
    return readValue(KEY_SELECTED_INTERFACES);
}

void AppSettings::setSelectedInterfaces(const QString& ids)
{
    writeValue(KEY_SELECTED_INTERFACES, ids.trimmed());
}

QString AppSettings::allowedCidrs() const
{
    return readValue(KEY_ALLOWED_CIDRS);
}

void AppSettings::setAllowedCidrs(const QString& cidrs)
{
    writeValue(KEY_ALLOWED_CIDRS, cidrs.trimmed());
}

QString AppSettings::networkMode() const
{
    return readValue(KEY_NETWORK_MODE, QStringLiteral("secure_lan"));
}

void AppSettings::setNetworkMode(const QString& mode)
{
    writeValue(KEY_NETWORK_MODE, mode.trimmed());
}

// ---- 向导状态 ----

bool AppSettings::onboardingCompleted() const
{
    return readValue(KEY_ONBOARDING_COMPLETED, "0") == "1";
}

void AppSettings::setOnboardingCompleted(bool completed)
{
    writeValue(KEY_ONBOARDING_COMPLETED, completed ? "1" : "0");
}

// ---- 内部辅助 ----

QString AppSettings::readValue(const QString& key, const QString& defaultValue) const
{
    if (!m_repository) {
        qWarning() << "AppSettings has no repository, returning default for key:" << key;
        return defaultValue;
    }

    return m_repository->value(key, defaultValue);
}

void AppSettings::writeValue(const QString& key, const QString& value)
{
    if (!m_repository) {
        qWarning() << "AppSettings has no repository, cannot write key:" << key;
        return;
    }

    if (m_repository->value(key) != value) {
        if (!m_repository->setValue(key, value)) {
            return;
        }
        emit settingChanged(key);
    }
}

} // namespace FengSui
