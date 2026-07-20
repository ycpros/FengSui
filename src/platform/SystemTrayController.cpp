#include "platform/SystemTrayController.h"

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
    if (!enabled || !QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }

    m_trayIcon = new QSystemTrayIcon(this);
    m_menu = new QMenu();

    QAction* openAction = m_menu->addAction(tr("打开 FengSui"));
    m_menu->addSeparator();
    QAction* quitAction = m_menu->addAction(tr("退出"));

    connect(openAction, &QAction::triggered,
            this, &SystemTrayController::restoreWindow);
    connect(quitAction, &QAction::triggered,
            QCoreApplication::instance(), &QCoreApplication::quit);
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
    m_available = true;
}

SystemTrayController::~SystemTrayController()
{
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
    if (m_available && m_trayIcon) {
        m_trayIcon->show();
    }
}

void SystemTrayController::restoreWindow()
{
    if (!m_window) {
        return;
    }

    m_window->showNormal();
    m_window->raise();
    m_window->requestActivate();
}

} // namespace FengSui
