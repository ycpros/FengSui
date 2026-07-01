// main.cpp
// FengSui 桌面客户端入口。
// 初始化 → 软件渲染回退检测 → 加载 QML 主壳 → 事件循环。
// 迁移到 QML 后不再实例化 QMainWindow，改用 QQmlApplicationEngine 加载 Main.qml。

#include "app/Application.h"
#include "app/AppSettings.h"
#include "ui/viewmodels/AppController.h"
#include "ui/viewmodels/ThemeController.h"

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

} // namespace

int main(int argc, char* argv[])
{
    // 使用 Basic 风格，让 Qt Quick Controls 完全受自定义主题控制，
    // 避免回退到平台原生风格（如 Windows 风格）与暗色主题冲突。
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    // 创建 FengSui 应用实例。UI 仍由 QML/Qt Quick 承载；QApplication 仅用于
    // 支撑 Windows 上 Qt.labs.platform SystemTrayIcon 的 QtWidgets 托盘后端。
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

    // models 层枚举通过 ui/viewmodels/QmlEnums.h 的 QML_FOREIGN_NAMESPACE 静态暴露为
    // QML 类型 Enums（属于 FengSui.Ui 模块），QML 中直接 import 后即可使用。

    // 构造顶层控制器（注入 Application）与主题控制器（注入 AppSettings），
    // 以单例形式注册给 QML。必须在 engine.load 之前完成注册。
    auto* appController = new FengSui::AppController(&app, &app);
    auto* themeController = new FengSui::ThemeController(&app);
    themeController->setAppSettings(app.settings());

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

    // 加载 QML 主壳
    QQmlApplicationEngine engine;
    engine.loadFromModule("FengSui.Ui", "Main");
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load QML root object, exiting";
        return 1;
    }

    // 开发用截图：--screenshot <路径> 加载后抓取窗口 PNG 并退出，
    // 便于在无人值守/无头环境下核对界面外观。可配合 --theme dark|light。
    const QStringList args = app.arguments();
    const int shotIdx = args.indexOf(QStringLiteral("--screenshot"));
    if (shotIdx >= 0 && shotIdx + 1 < args.size()) {
        const QString outPath = args.at(shotIdx + 1);
        const int themeIdx = args.indexOf(QStringLiteral("--theme"));
        if (themeIdx >= 0 && themeIdx + 1 < args.size()) {
            themeController->setMode(args.at(themeIdx + 1));
        }
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        if (window) {
            // 等待场景图完成首帧再抓取，避免截到空白。
            QTimer::singleShot(800, window, [window, outPath]() {
                QDir().mkpath(QFileInfo(outPath).absolutePath());
                const QImage img = window->grabWindow();
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
