// SettingsViewModel.cpp
// 设置页 ViewModel 实现。

#include "ui/viewmodels/SettingsViewModel.h"

#include "app/Application.h"
#include "app/AppSettings.h"
#include "core/BeaconService.h"
#include "models/NetworkPolicy.h"
#include "platform/InterfaceEnumerator.h"

#include <QVariantMap>

namespace FengSui {

namespace {
QString typeText(InterfaceType type)
{
    switch (type) {
    case InterfaceType::Physical:  return QStringLiteral("物理");
    case InterfaceType::Virtual:   return QStringLiteral("虚拟");
    case InterfaceType::Loopback:  return QStringLiteral("回环");
    case InterfaceType::LinkLocal: return QStringLiteral("链路本地");
    case InterfaceType::Public:    return QStringLiteral("公网");
    default:                       return QStringLiteral("未知");
    }
}

QString modeDisplayText(const QString& mode)
{
    if (mode == QLatin1String("multi_lan"))   return QStringLiteral("多网卡内网");
    if (mode == QLatin1String("compat_test")) return QStringLiteral("兼容测试");
    return QStringLiteral("安全内网");
}
} // namespace

SettingsViewModel::SettingsViewModel(QObject* parent)
    : QObject(parent)
{
}

void SettingsViewModel::bind(Application* app)
{
    m_app = app;
    m_settings = app ? app->settings() : nullptr;
    // 全量通知一次，让 QML 初始读取
    emit displayNameChanged();
    emit downloadDirChanged();
    emit autoStartChanged();
    emit minimizeToTrayChanged();
    emit discoveryEnabledChanged();
    emit listenPortChanged();
    emit networkModeChanged();
    emit allowedCidrsChanged();
    emit interfacesChanged();
    emit diagChanged();
}

// ---- 常规 ----

QString SettingsViewModel::displayName() const
{
    return m_settings ? m_settings->displayName() : QString();
}
void SettingsViewModel::setDisplayName(const QString& v)
{
    if (!m_settings || v == displayName()) return;
    m_settings->setDisplayName(v);
    emit displayNameChanged();
}

QString SettingsViewModel::downloadDir() const
{
    return m_settings ? m_settings->downloadDir() : QString();
}
void SettingsViewModel::setDownloadDir(const QString& v)
{
    if (!m_settings || v == downloadDir()) return;
    m_settings->setDownloadDir(v);
    emit downloadDirChanged();
}

bool SettingsViewModel::autoStart() const
{
    return m_settings && m_settings->autoStart();
}
void SettingsViewModel::setAutoStart(bool v)
{
    if (!m_settings || v == autoStart()) return;
    m_settings->setAutoStart(v);
    emit autoStartChanged();
}

bool SettingsViewModel::minimizeToTray() const
{
    return m_settings && m_settings->minimizeToTray();
}
void SettingsViewModel::setMinimizeToTray(bool v)
{
    if (!m_settings || v == minimizeToTray()) return;
    m_settings->setMinimizeToTray(v);
    emit minimizeToTrayChanged();
}

// ---- 网络 ----

bool SettingsViewModel::discoveryEnabled() const
{
    return m_settings && m_settings->discoveryEnabled();
}
void SettingsViewModel::setDiscoveryEnabled(bool v)
{
    if (!m_settings || v == discoveryEnabled()) return;
    m_settings->setDiscoveryEnabled(v);
    emit discoveryEnabledChanged();
    markNeedsRestart();
}

int SettingsViewModel::listenPort() const
{
    return m_settings ? m_settings->listenPort() : 8787;
}
void SettingsViewModel::setListenPort(int v)
{
    if (!m_settings || v == listenPort() || v < 1 || v > 65535) return;
    m_settings->setListenPort(static_cast<quint16>(v));
    emit listenPortChanged();
    markNeedsRestart();
}

QString SettingsViewModel::networkMode() const
{
    return m_settings ? m_settings->networkMode() : QStringLiteral("secure_lan");
}
void SettingsViewModel::setNetworkMode(const QString& v)
{
    if (!m_settings || v == networkMode()) return;
    m_settings->setNetworkMode(v);
    emit networkModeChanged();
    markNeedsRestart();
}

QString SettingsViewModel::allowedCidrs() const
{
    return m_settings ? m_settings->allowedCidrs() : QString();
}
void SettingsViewModel::setAllowedCidrs(const QString& v)
{
    if (!m_settings || v == allowedCidrs()) return;
    m_settings->setAllowedCidrs(v);
    emit allowedCidrsChanged();
    markNeedsRestart();
}

// ---- 网卡列表 ----

QStringList SettingsViewModel::selectedInterfaceIds() const
{
    if (!m_settings) return {};
    return m_settings->selectedInterfaces().split(QLatin1Char(','), Qt::SkipEmptyParts);
}

QVariantList SettingsViewModel::interfaces() const
{
    QVariantList out;
    const QStringList selected = selectedInterfaceIds();
    const QList<NetworkInterfaceInfo> ifaces = InterfaceEnumerator::enumerate();
    for (const NetworkInterfaceInfo& i : ifaces) {
        QVariantMap m;
        m["id"] = i.id;
        m["name"] = i.name;
        m["displayName"] = i.displayName;
        m["cidr"] = i.cidr();
        m["type"] = typeText(i.type);
        m["physical"] = i.isPhysical;
        m["selected"] = selected.contains(i.id);
        out.append(m);
    }
    return out;
}

void SettingsViewModel::toggleInterface(const QString& id, bool selected)
{
    if (!m_settings) return;
    QStringList ids = selectedInterfaceIds();
    if (selected) {
        if (!ids.contains(id)) ids.append(id);
    } else {
        ids.removeAll(id);
    }
    m_settings->setSelectedInterfaces(ids.join(QLatin1Char(',')));
    emit interfacesChanged();
    markNeedsRestart();
}

// ---- 诊断 ----

QString SettingsViewModel::diagMode() const
{
    return modeDisplayText(networkMode());
}
int SettingsViewModel::diagPort() const
{
    return listenPort();
}
QString SettingsViewModel::diagAllowedCidrs() const
{
    if (m_app && m_app->networkPolicy()) {
        const QStringList cidrs = m_app->networkPolicy()->allowedCidrs();
        return cidrs.isEmpty() ? QStringLiteral("（无）") : cidrs.join(QStringLiteral(", "));
    }
    const QString c = allowedCidrs();
    return c.isEmpty() ? QStringLiteral("（无）") : c;
}
int SettingsViewModel::diagOnlineCount() const
{
    if (m_app && m_app->beaconService()) {
        return m_app->beaconService()->peers().size();
    }
    return 0;
}

void SettingsViewModel::refresh()
{
    emit interfacesChanged();
    emit diagChanged();
}

void SettingsViewModel::markNeedsRestart()
{
    if (!m_needsRestart) {
        m_needsRestart = true;
        emit needsRestartChanged();
    }
}

} // namespace FengSui
