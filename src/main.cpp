// main.cpp
// FengSui 桌面客户端入口。
// 初始化 → 软件渲染回退检测 → 加载 QML 主壳 → 事件循环。
// 迁移到 QML 后改用 QQmlApplicationEngine 加载 Main.qml。

#include "app/Application.h"
#include "app/AppSettings.h"
#include "platform/SystemTrayController.h"
#include "ui/viewmodels/AppController.h"
#include "ui/viewmodels/ThemeController.h"
#include "Version.h"

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QTimer>
#include <QImage>
#include <QDir>
#include <QFileInfo>

#include <cstdio>

namespace {

// 探测系统是否具备可用的 OpenGL 上下文。
// 无 GPU / 远程桌面 / 老旧驱动环境下返回 false，此时回退到软件渲染，
// 避免场景图初始化失败导致白屏或崩溃。
bool hasUsableOpenGL()
{
    if (!QApplication::primaryScreen()) {
        return false;
    }

    QOpenGLContext context;
    if (!context.create()) {
        return false;
    }

    QOffscreenSurface surface;
    surface.setFormat(context.format());
    surface.create();
    if (!surface.isValid()) {
        return false;
    }

    // 能创建上下文还不够，需确认能真正 makeCurrent
    const bool ok = context.makeCurrent(&surface);
    context.doneCurrent();
    return ok;
}

int screenshotPageIndex(const QString& pageName)
{
    if (pageName == QStringLiteral("chat")) {
        return 0;
    }
    if (pageName == QStringLiteral("contacts")) {
        return 1;
    }
    if (pageName == QStringLiteral("transfer")) {
        return 2;
    }
    if (pageName == QStringLiteral("share")) {
        return 3;
    }
    if (pageName == QStringLiteral("settings")) {
        return 4;
    }
    return -1;
}

} // namespace

int main(int argc, char* argv[])
{
    // --version / -v：打印版本号后退出（不需要启动 QApplication 和 QML 引擎）
    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == QStringLiteral("--version") || arg == QStringLiteral("-v")) {
            std::fprintf(stdout, "FengSui %s\n", FENGSUI_VERSION_STRING);
            std::fflush(stdout);
            return 0;
        }
    }

    // 使用 Basic 风格，让 Qt Quick Controls 完全受自定义主题控制，
    // 避免回退到平台原生风格（如 Windows 风格）与暗色主题冲突。
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    // 创建 FengSui 应用实例。UI 仍由 QML/Qt Quick 承载；QApplication 同时
    // 支撑 SystemTrayController 使用的原生 QSystemTrayIcon。
    FengSui::Application app(argc, argv);

    // QOffscreenSurface 依赖 QApplication 初始化出的 platform/screen。
    // 因此 OpenGL 探测放在 app 构造之后，但仍早于任何 QQuickWindow/场景图创建。
    if (qEnvironmentVariableIsEmpty("QT_QUICK_BACKEND")) {
        if (!hasUsableOpenGL()) {
            QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
            qputenv("QT_QUICK_BACKEND", "software");
        }
    }

    // 初始化子系统（日志、设置、数据库等）
    if (!app.initialize()) {
        qCritical() << "Application initialization failed, exiting";
        return 1;
    }
    const QStringList args = app.arguments();
    const int shotIdx = args.indexOf(QStringLiteral("--screenshot"));
    const bool screenshotRequested = shotIdx >= 0 && shotIdx + 1 < args.size();

    // models 层枚举通过 ui/viewmodels/QmlEnums.h 的 QML_FOREIGN_NAMESPACE 静态暴露为
    // QML 类型 Enums（属于 FengSui.Ui 模块），QML 中直接 import 后即可使用。

    // 构造顶层控制器（注入 Application）与主题控制器（注入 AppSettings），
    // 以单例形式注册给 QML。必须在 engine.load 之前完成注册。
    auto* appController = new FengSui::AppController(&app, &app);
    auto* themeController = new FengSui::ThemeController(&app);
    auto* systemTray = new FengSui::SystemTrayController(!screenshotRequested, &app);
    themeController->setAppSettings(app.settings());

    if (screenshotRequested) {
        appController->setOnboardingSuppressed(true);

        const int pageIdx = args.indexOf(QStringLiteral("--screenshot-page"));
        if (pageIdx >= 0) {
            if (pageIdx + 1 >= args.size()) {
                qWarning() << "screenshot-page: missing page name";
                return 2;
            }

            const QString pageName = args.at(pageIdx + 1).trimmed().toLower();
            const int targetIndex = screenshotPageIndex(pageName);
            if (targetIndex < 0) {
                qWarning() << "screenshot-page: unknown page" << pageName
                           << "(expected chat, contacts, transfer, share, or settings)";
                return 2;
            }
            appController->setCurrentIndex(targetIndex);
        }

        const int themeIdx = args.indexOf(QStringLiteral("--theme"));
        if (themeIdx >= 0 && themeIdx + 1 < args.size()) {
            themeController->setMode(args.at(themeIdx + 1));
        }
    }

    // 首次向导未完成时，不在此启动服务：由 QML 向导页在 finish 后经
    // AppController::completeOnboarding 刷新策略、启动服务并绑定各子 ViewModel。
    // 已完成向导则直接启动并绑定。
    if (app.settings()->onboardingCompleted()) {
        app.reloadNetworkPolicyFromSettings();
        app.startBeaconService();
        app.startSignalService();
        app.startCourierService();
        appController->bindServices();
    }

    qmlRegisterSingletonInstance("FengSui.Ui", 1, 0, "AppController", appController);
    qmlRegisterSingletonInstance("FengSui.Ui", 1, 0, "ThemeController", themeController);
    qmlRegisterSingletonInstance("FengSui.Ui", 1, 0, "SystemTray", systemTray);

    // 加载 QML 主壳
    QQmlApplicationEngine engine;
    engine.loadFromModule("FengSui.Ui", "Main");
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load QML root object, exiting";
        return 1;
    }

    auto* rootWindow = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    if (!rootWindow) {
        qCritical() << "QML root object is not a QQuickWindow, exiting";
        return 1;
    }
    systemTray->attachWindow(rootWindow);

    // 开发用截图：--screenshot <路径> 加载后抓取窗口 PNG 并退出，
    // 便于在无人值守/无头环境下核对界面外观。可配合 --theme dark|light。
    if (screenshotRequested) {
        const QString outPath = args.at(shotIdx + 1);
        if (rootWindow) {
            // 等待场景图完成首帧再抓取，避免截到空白。
            QTimer::singleShot(800, rootWindow, [rootWindow, outPath]() {
                QDir().mkpath(QFileInfo(outPath).absolutePath());
                const QImage img = rootWindow->grabWindow();
                const bool ok = !img.isNull() && img.save(outPath);
                qInfo() << "screenshot" << (ok ? "saved" : "FAILED") << outPath;
                QApplication::exit(ok ? 0 : 2);
            });
        } else {
            qWarning() << "screenshot: root is not a QQuickWindow";
            return 2;
        }
    }

    return app.exec();
}
