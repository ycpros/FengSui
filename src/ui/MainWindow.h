// MainWindow.h
// 主窗口：左侧导航栏 + 右侧 QStackedWidget 页面栈。
// 导航项：消息（聊天）、联系人、传输中心、共享文件、设置。
// 不直接操作网络或存储，通过 signal/slot 与 core 层交互。

#pragma once

#include <QMainWindow>

class QListWidget;
class QListWidgetItem;
class QStackedWidget;

namespace FengSui {

class Application;
class ChatPage;
class ContactsPage;
class TransferCenterPage;
class SharePage;
class SettingsPage;

// 主窗口：左侧 QListWidget 导航（260px）+ 右侧 QStackedWidget。
// 页面切换通过导航选中信号驱动，最小窗口 900×600。
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    // 构造时创建全部子页面，但不加载数据。
    explicit MainWindow(Application* app, QWidget* parent = nullptr);

    // 返回当前选中的页面索引（0=消息, 1=联系人, 2=传输中心, 3=共享文件, 4=设置）
    int currentPageIndex() const;

    // 切换到指定页面
    void switchToPage(int index);

private slots:
    // 导航列表选中项变化时切换页面栈
    void onNavItemChanged(QListWidgetItem* current, QListWidgetItem* previous);

private:
    // 创建界面布局
    void setupUi();

    // 创建左侧导航栏（图标+文字）
    void createNavBar();

    // 创建右侧页面栈
    void createPageStack();

    Application* m_app = nullptr;
    QListWidget*   m_navList = nullptr;
    QStackedWidget* m_pageStack = nullptr;

    // 5 个功能页面（由 MainWindow 拥有）
    ChatPage*           m_chatPage = nullptr;
    ContactsPage*       m_contactsPage = nullptr;
    TransferCenterPage* m_transferPage = nullptr;
    SharePage*          m_sharePage = nullptr;
    SettingsPage*       m_settingsPage = nullptr;
};

} // namespace FengSui
