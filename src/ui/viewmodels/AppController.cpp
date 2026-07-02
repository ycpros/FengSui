// AppController.cpp
// 顶层壳控制器实现。

#include "ui/viewmodels/AppController.h"

#include "app/Application.h"
#include "app/AppSettings.h"
#include "ui/viewmodels/ContactsViewModel.h"
#include "ui/viewmodels/TransferViewModel.h"
#include "ui/viewmodels/SettingsViewModel.h"
#include "ui/viewmodels/ShareViewModel.h"
#include "ui/viewmodels/ChatViewModel.h"
#include "ui/viewmodels/OnboardingViewModel.h"

namespace FengSui {

AppController::AppController(Application* app, QObject* parent)
    : QObject(parent)
    , m_app(app)
    , m_contacts(new ContactsViewModel(this))
    , m_transfer(new TransferViewModel(this))
    , m_settings(new SettingsViewModel(this))
    , m_share(new ShareViewModel(this))
    , m_chat(new ChatViewModel(this))
    , m_onboarding(new OnboardingViewModel(this))
{
    // 联系人页双击设备 → 打开会话并切到消息页
    connect(m_contacts, &ContactsViewModel::requestOpenConversation,
            this, [this](const QString& peerId) {
                m_chat->openConversation(peerId);
                setCurrentIndex(0);
            });

    // 向导完成 → 走服务启动与绑定时序
    connect(m_onboarding, &OnboardingViewModel::finished,
            this, &AppController::completeOnboarding);

    // 向导只依赖 AppSettings，可立即绑定（服务未启动也能收集配置）
    m_onboarding->bind(m_app);
}

int AppController::currentIndex() const
{
    return m_currentIndex;
}

void AppController::setCurrentIndex(int index)
{
    if (index == m_currentIndex) {
        return;
    }
    m_currentIndex = index;
    emit currentIndexChanged();
}

bool AppController::onboardingNeeded() const
{
    if (m_onboardingSuppressed) {
        return false;
    }
    if (!m_app || !m_app->settings()) {
        return false;
    }
    return !m_app->settings()->onboardingCompleted();
}

void AppController::setOnboardingSuppressed(bool suppressed)
{
    if (suppressed == m_onboardingSuppressed) {
        return;
    }
    m_onboardingSuppressed = suppressed;
    emit onboardingNeededChanged();
}

QString AppController::displayName() const
{
    if (!m_app || !m_app->settings()) {
        return QString();
    }
    return m_app->settings()->displayName();
}

void AppController::completeOnboarding()
{
    if (!m_app || !m_app->settings()) {
        return;
    }

    m_app->settings()->setOnboardingCompleted(true);

    // 向导可能刚写入授权网卡与 CIDR，启动服务前刷新运行时策略。
    m_app->reloadNetworkPolicyFromSettings();
    m_app->startBeaconService();
    m_app->startSignalService();
    m_app->startCourierService();

    bindServices();

    emit onboardingNeededChanged();
    emit displayNameChanged();
}

void AppController::bindServices()
{
    if (!m_app) {
        return;
    }
    m_contacts->bind(m_app->beaconService(),
                     m_app->networkPolicy(),
                     m_app->manualPeerRepository());
    m_transfer->bind(m_app->courierService(),
                     m_app->beaconService());
    m_settings->bind(m_app);
    m_share->bind(m_app->shareService(),
                  m_app->beaconService());
    m_chat->bind(m_app->signalService(),
                 m_app->courierService(),
                 m_app->beaconService(),
                 m_app->settings() ? m_app->settings()->peerId() : QString());
}

} // namespace FengSui
