// main.cpp
// FengSui 桌面客户端入口。
// 初始化 → 向导检查 → 主窗口 → 事件循环。

#include "app/Application.h"
#include "app/AppSettings.h"
#include "ui/MainWindow.h"
#include "ui/onboarding/OnboardingWizard.h"

int main(int argc, char* argv[])
{
    // 创建 FengSui 应用实例
    FengSui::Application app(argc, argv);

    // 初始化子系统（日志、设置、数据库等）
    if (!app.initialize()) {
        qCritical() << "Application initialization failed, exiting";
        return 1;
    }

    // 检查是否已完成首次启动向导
    if (!app.settings()->onboardingCompleted()) {
        // 弹出向导，阻塞等待用户完成或取消
        FengSui::OnboardingWizard wizard(app.settings());
        int result = wizard.exec();

        // 用户取消或关闭向导 → 退出应用
        if (result == QDialog::Rejected) {
            qInfo() << "Onboarding cancelled by user, exiting";
            return 0;
        }

        // QWizard::accept() 已在 OnboardingWizard::accept() 中保存设置
        qInfo() << "Onboarding completed";
    }

    // 向导完成后再启动服务，避免首次设置阶段使用默认配置。
    app.startBeaconService();
    app.startSignalService();
    app.startCourierService();

    // 创建并显示主窗口
    auto* mainWindow = new FengSui::MainWindow(&app);
    mainWindow->show();

    // 进入 Qt 事件循环
    int result = app.exec();

    delete mainWindow;
    return result;
}
