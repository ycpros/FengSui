#pragma once

#include <QObject>
#include <QPointer>

class QMenu;
class QQuickWindow;
class QSystemTrayIcon;

namespace FengSui {

class SystemTrayController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)

public:
    explicit SystemTrayController(bool enabled, QObject* parent = nullptr);
    ~SystemTrayController() override;

    bool available() const;
    void attachWindow(QQuickWindow* window);

private:
    void restoreWindow();

    bool                   m_available = false;
    QPointer<QQuickWindow> m_window;
    QSystemTrayIcon*       m_trayIcon = nullptr;
    QMenu*                 m_menu = nullptr;
};

} // namespace FengSui
