#include "platform/SystemTrayController.h"

// 托盘控制器只依赖公开的 Qt Widgets API，不依赖 Qt.labs.platform QML 插件。
#include <QAction>
#include <QCoreApplication>
#include <QIcon>
#include <QMenu>
#include <QQuickWindow>
#include <QSystemTrayIcon>

namespace FengSui {

SystemTrayController::SystemTrayController(bool enabled, QObject* parent)
    : QObject(parent)
{
    // 截图模式主动禁用托盘；无系统托盘的桌面环境也保持 unavailable。调用方据此让
    // 关闭窗口执行正常退出，避免进程隐藏后没有任何恢复入口。
    if (!enabled || !QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }

    m_trayIcon = new QSystemTrayIcon(this);
    // QSystemTrayIcon::setContextMenu() 不转移菜单所有权，因此菜单不设置 QObject
    // parent，并由本类析构函数显式删除。
    m_menu = new QMenu();

    // 菜单命令保持最小且可预测：“打开”恢复同一主窗口，“退出”结束整个应用。
    QAction* openAction = m_menu->addAction(tr("打开 FengSui"));
    m_menu->addSeparator();
    QAction* quitAction = m_menu->addAction(tr("退出"));

    connect(openAction, &QAction::triggered,
            this, &SystemTrayController::restoreWindow);
    connect(quitAction, &QAction::triggered,
            QCoreApplication::instance(), &QCoreApplication::quit);

    // Windows 上单击和双击都可能作为用户恢复窗口的自然操作。忽略右键、菜单等
    // 其他激活原因，防止打开上下文菜单时意外抢占前台焦点。
    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this, [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger
                    || reason == QSystemTrayIcon::DoubleClick) {
                    restoreWindow();
                }
            });

    const QIcon icon(QStringLiteral(":/qt/qml/FengSui/Ui/assets/tray.svg"));
    m_trayIcon->setIcon(icon);
    m_trayIcon->setToolTip(tr("烽燧 FengSui"));
    m_trayIcon->setContextMenu(m_menu);

    // 所有资源和信号连接准备完成后才公开 available，避免 QML 隐藏窗口时托盘尚未
    // 具备恢复能力。图标本身延迟到 attachWindow() 后显示。
    m_available = true;
}

SystemTrayController::~SystemTrayController()
{
    // 先从系统托盘移除图标，再销毁菜单，避免退出期间残留图标或菜单回调。
    if (m_trayIcon) {
        m_trayIcon->hide();
    }
    delete m_menu;
}

bool SystemTrayController::available() const
{
    return m_available;
}

void SystemTrayController::attachWindow(QQuickWindow* window)
{
    m_window = window;

    // 必须等 QML 根窗口加载成功后再显示托盘。这样托盘一旦可见，单击“打开”始终
    // 已有明确的恢复目标。
    if (m_available && m_trayIcon) {
        m_trayIcon->show();
    }
}

void SystemTrayController::restoreWindow()
{
    if (!m_window) {
        return;
    }

    // showNormal() 同时覆盖隐藏和最小化状态；raise() 与 requestActivate() 配合请求
    // 窗口管理器把应用带回前台。部分平台可能拒绝强制激活，但窗口仍会恢复可见。
    m_window->showNormal();
    m_window->raise();
    m_window->requestActivate();
}

} // namespace FengSui
