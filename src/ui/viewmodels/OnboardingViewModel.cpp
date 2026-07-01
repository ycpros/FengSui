// OnboardingViewModel.cpp

#include "ui/viewmodels/OnboardingViewModel.h"

#include "app/Application.h"
#include "app/AppSettings.h"
#include "platform/InterfaceEnumerator.h"

#include <QVariantMap>

namespace FengSui {

OnboardingViewModel::OnboardingViewModel(QObject* parent)
    : QObject(parent)
{
}

void OnboardingViewModel::bind(Application* app)
{
    m_app = app;
    m_settings = app ? app->settings() : nullptr;
    if (m_settings) {
        m_nickname = m_settings->displayName();
        m_discoveryEnabled = m_settings->discoveryEnabled();
        m_downloadDir = m_settings->downloadDir();
    }
    // 默认选中首个候选物理网卡
    const QList<NetworkInterfaceInfo> cands = InterfaceEnumerator::candidates();
    if (!cands.isEmpty()) {
        m_selectedIds = { cands.first().id };
    }
    emit nicknameChanged();
    emit discoveryEnabledChanged();
    emit downloadDirChanged();
    emit interfacesChanged();
}

void OnboardingViewModel::setNickname(const QString& v)
{
    if (v == m_nickname) return;
    m_nickname = v;
    emit nicknameChanged();
}

void OnboardingViewModel::setDiscoveryEnabled(bool v)
{
    if (v == m_discoveryEnabled) return;
    m_discoveryEnabled = v;
    emit discoveryEnabledChanged();
}

void OnboardingViewModel::setDownloadDir(const QString& v)
{
    if (v == m_downloadDir) return;
    m_downloadDir = v;
    emit downloadDirChanged();
}

QVariantList OnboardingViewModel::interfaces() const
{
    QVariantList out;
    const QList<NetworkInterfaceInfo> cands = InterfaceEnumerator::candidates();
    for (const NetworkInterfaceInfo& i : cands) {
        QVariantMap m;
        m["id"] = i.id;
        m["name"] = i.name;
        m["cidr"] = i.cidr();
        m["physical"] = i.isPhysical;
        m["selected"] = m_selectedIds.contains(i.id);
        out.append(m);
    }
    return out;
}

bool OnboardingViewModel::needsInterfaceStep() const
{
    // 多于一个候选网卡时才需要用户手动选择
    return InterfaceEnumerator::candidates().size() > 1;
}

void OnboardingViewModel::toggleInterface(const QString& id, bool selected)
{
    if (selected) {
        if (!m_selectedIds.contains(id)) m_selectedIds.append(id);
    } else {
        m_selectedIds.removeAll(id);
    }
    emit interfacesChanged();
}

void OnboardingViewModel::finish()
{
    if (m_settings) {
        m_settings->setDisplayName(m_nickname.trimmed());
        m_settings->setDiscoveryEnabled(m_discoveryEnabled);
        m_settings->setDownloadDir(m_downloadDir);
        m_settings->setSelectedInterfaces(m_selectedIds.join(QLatin1Char(',')));
    }
    emit finished();   // AppController 接此信号执行 completeOnboarding
}

} // namespace FengSui
