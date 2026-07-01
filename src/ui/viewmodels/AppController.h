// AppController.h
// 顶层壳控制器：管理主导航索引、首次向导时序，并聚合各功能子 ViewModel。
// 由 main.cpp 构造（注入 Application），以单例形式注册给 QML。
// 阶段 2 仅含导航与向导状态；阶段 3 起把 chat/contacts/transfer/share/settings 等
// 子 ViewModel 作为属性挂到这里，QML 通过 AppController.chat.xxx 访问。

#pragma once

#include "ui/viewmodels/ContactsViewModel.h"
#include "ui/viewmodels/TransferViewModel.h"
#include "ui/viewmodels/SettingsViewModel.h"
#include "ui/viewmodels/ShareViewModel.h"
#include "ui/viewmodels/ChatViewModel.h"
#include "ui/viewmodels/OnboardingViewModel.h"

#include <QObject>

namespace FengSui {

class Application;

class AppController : public QObject {
    Q_OBJECT

    // 当前主导航页索引（0 消息 / 1 联系人 / 2 传输中心 / 3 共享文件 / 4 设置）
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    // 是否需要显示首次启动向导
    Q_PROPERTY(bool onboardingNeeded READ onboardingNeeded NOTIFY onboardingNeededChanged)
    // 本机昵称（导航栏顶部展示）
    Q_PROPERTY(QString displayName READ displayName NOTIFY displayNameChanged)

    // 子 ViewModel（QML 通过 AppController.contacts.xxx 访问）
    Q_PROPERTY(ContactsViewModel* contacts READ contacts CONSTANT)
    Q_PROPERTY(TransferViewModel* transfer READ transfer CONSTANT)
    Q_PROPERTY(SettingsViewModel* settings READ settingsVm CONSTANT)
    Q_PROPERTY(ShareViewModel* share READ share CONSTANT)
    Q_PROPERTY(ChatViewModel* chat READ chat CONSTANT)
    Q_PROPERTY(OnboardingViewModel* onboarding READ onboarding CONSTANT)

public:
    explicit AppController(Application* app, QObject* parent = nullptr);

    int currentIndex() const;
    void setCurrentIndex(int index);

    bool onboardingNeeded() const;

    QString displayName() const;

    ContactsViewModel* contacts() const { return m_contacts; }
    TransferViewModel* transfer() const { return m_transfer; }
    SettingsViewModel* settingsVm() const { return m_settings; }
    ShareViewModel* share() const { return m_share; }
    ChatViewModel* chat() const { return m_chat; }
    OnboardingViewModel* onboarding() const { return m_onboarding; }

    // 首次向导完成回调：写入完成标志、刷新网络策略、启动服务、绑定子 VM。
    // 阶段 4 由 QML 向导页在 finished 时调用。
    Q_INVOKABLE void completeOnboarding();

    // 把已启动的 Service 注入各子 ViewModel 并连接信号。
    // 服务就绪后调用（当前跳过向导流程下由 main 在启动服务后调用；
    // 阶段 4 改由 completeOnboarding 调用）。
    void bindServices();

signals:
    void currentIndexChanged();
    void onboardingNeededChanged();
    void displayNameChanged();

private:
    Application*        m_app = nullptr;
    int                m_currentIndex = 0;
    ContactsViewModel* m_contacts = nullptr;
    TransferViewModel* m_transfer = nullptr;
    SettingsViewModel* m_settings = nullptr;
    ShareViewModel*      m_share = nullptr;
    ChatViewModel*       m_chat = nullptr;
    OnboardingViewModel* m_onboarding = nullptr;
};

} // namespace FengSui
