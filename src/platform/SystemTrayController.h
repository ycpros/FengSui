// SystemTrayController.h
// 使用公开的 Qt Widgets QSystemTrayIcon 管理系统托盘，并向 QML 暴露只读可用状态。
// 托盘菜单和窗口恢复逻辑集中在 C++，QML 只负责依据 available 决定关闭时是否隐藏。

#pragma once

#include <QObject>
#include <QPointer>

class QMenu;
class QQuickWindow;
class QSystemTrayIcon;

namespace FengSui {

// 系统托盘控制器。
// 由 main.cpp 在 QML 加载前构造并注册为 SystemTray 单例。控制器负责托盘图标、
// 上下文菜单和主窗口恢复；不读取 AppSettings，也不决定窗口是否应最小化到托盘。
// 主窗口由 attachWindow() 延迟注入，避免控制器依赖 QML 根对象的创建顺序。
// 线程安全性：QSystemTrayIcon、QMenu 和 QQuickWindow 均要求在 GUI 主线程使用。
class SystemTrayController final : public QObject {
    Q_OBJECT

    // 系统托盘后端是否可用且已经成功初始化。截图模式或系统无托盘时为 false。
    Q_PROPERTY(bool available READ available CONSTANT)

public:
    // 创建系统托盘控制器。
    // enabled: 是否允许创建托盘。截图模式传入 false，避免截图进程污染系统托盘。
    // parent: Qt 父对象，通常为 Application；其生命周期必须覆盖 QML 引擎和主窗口。
    // enabled 为 false 或系统不支持托盘时，构造仍成功，但 available() 返回 false。
    explicit SystemTrayController(bool enabled, QObject* parent = nullptr);

    // 隐藏托盘图标并释放 QSystemTrayIcon 不会接管所有权的上下文菜单。
    ~SystemTrayController() override;

    // 返回托盘是否可供 QML 的“关闭时隐藏”逻辑使用。
    // 返回值：托盘后端初始化成功时为 true，否则为 false；对象生命周期内保持不变。
    bool available() const;

    // 绑定需要由托盘恢复的 QML 主窗口，并在托盘可用时显示图标。
    // window: QML 引擎创建的根 QQuickWindow，可以为 nullptr。窗口通过 QPointer 保存，
    // 销毁后会自动清空，托盘激活不会访问悬空指针。
    // 线程安全性：仅在 GUI 主线程、QML 根对象加载成功后调用。
    void attachWindow(QQuickWindow* window);

private:
    // 将已绑定窗口恢复为正常状态、提升到前台并请求输入焦点。
    // 未绑定窗口或窗口已经销毁时不执行任何操作。
    void restoreWindow();

    // available 是 QML 只读常量，只在构造阶段完成初始化后置为 true。
    bool                   m_available = false;
    // QML 引擎拥有窗口；QPointer 用于观察生命周期，不转移所有权。
    QPointer<QQuickWindow> m_window;
    // QObject 父子关系由本控制器拥有并释放托盘图标。
    QSystemTrayIcon*       m_trayIcon = nullptr;
    // QSystemTrayIcon 不拥有 contextMenu，析构函数负责显式释放。
    QMenu*                 m_menu = nullptr;
};

} // namespace FengSui
