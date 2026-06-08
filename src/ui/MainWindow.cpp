// MainWindow.cpp
// 主窗口实现：左侧导航 + 右侧页面栈布局。

#include "ui/MainWindow.h"
#include "app/AppSettings.h"
#include "app/Application.h"
#include "core/SignalService.h"
#include "ui/chat/ChatPage.h"
#include "ui/contacts/ContactsPage.h"
#include "ui/transfer_center/TransferCenterPage.h"
#include "ui/share/SharePage.h"
#include "ui/settings/SettingsPage.h"

#include <QHBoxLayout>
#include <QListWidget>
#include <QStackedWidget>

namespace FengSui {

// 导航项定义：显示名称 + 对应的页面索引
static const struct {
    const char* label;
    int         pageIndex;
} kNavItems[] = {
    { "\xe6\xb6\x88\xe6\x81\xaf", 0 },       // 消息
    { "\xe8\x81\x94\xe7\xb3\xbb\xe4\xba\xba", 1 }, // 联系人
    { "\xe4\xbc\xa0\xe8\xbe\x93\xe4\xb8\xad\xe5\xbf\x83", 2 }, // 传输中心
    { "\xe5\x85\xb1\xe4\xba\xab\xe6\x96\x87\xe4\xbb\xb6", 3 }, // 共享文件
    { "\xe8\xae\xbe\xe7\xbd\xae", 4 },       // 设置
};

MainWindow::MainWindow(Application* app, QWidget* parent)
    : QMainWindow(parent)
    , m_app(app)
{
    setWindowTitle(QString::fromUtf8("\xe7\x83\xbd\xe7\x87\xa7 FengSui"));
    setMinimumSize(900, 600);
    resize(1100, 720);

    setupUi();
}

int MainWindow::currentPageIndex() const
{
    return m_pageStack ? m_pageStack->currentIndex() : -1;
}

void MainWindow::switchToPage(int index)
{
    if (m_pageStack && index >= 0 && index < m_pageStack->count()) {
        m_pageStack->setCurrentIndex(index);
        if (m_navList && index < m_navList->count()) {
            m_navList->setCurrentRow(index);
        }
    }
}

void MainWindow::onNavItemChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
    static_cast<void>(previous);

    if (!current || !m_pageStack) return;

    int index = current->data(Qt::UserRole).toInt();
    m_pageStack->setCurrentIndex(index);
}

void MainWindow::setupUi()
{
    // 中央部件：水平布局（导航 + 页面栈）
    auto* centralWidget = new QWidget(this);
    auto* layout = new QHBoxLayout(centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    createNavBar();
    createPageStack();

    layout->addWidget(m_navList);
    layout->addWidget(m_pageStack, 1);  // 页面栈占满剩余空间

    setCentralWidget(centralWidget);

    // 默认选中第一项（消息）
    if (m_navList->count() > 0) {
        m_navList->setCurrentRow(0);
    }
}

void MainWindow::createNavBar()
{
    m_navList = new QListWidget(this);
    m_navList->setFixedWidth(260);
    m_navList->setFocusPolicy(Qt::NoFocus);  // 导航栏不抢键盘焦点

    // 导航项样式：选中项加粗
    m_navList->setStyleSheet(
        "QListWidget {"
        "  border: none;"
        "  background-color: #f5f5f5;"
        "  padding: 8px 0;"
        "}"
        "QListWidget::item {"
        "  padding: 12px 20px;"
        "  font-size: 14px;"
        "  border: none;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #e0e0e0;"
        "  font-weight: bold;"
        "  color: #333;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: #ebebeb;"
        "}"
    );

    // 添加 5 个导航项
    for (const auto& nav : kNavItems) {
        auto* item = new QListWidgetItem(QString::fromUtf8(nav.label), m_navList);
        item->setData(Qt::UserRole, nav.pageIndex);  // 存储页面索引
        // 设置导航项高度
        item->setSizeHint(QSize(0, 48));
    }

    connect(m_navList, &QListWidget::currentItemChanged,
            this, &MainWindow::onNavItemChanged);
}

void MainWindow::createPageStack()
{
    m_pageStack = new QStackedWidget(this);

    // 按导航顺序创建 5 个页面
    m_chatPage     = new ChatPage(this);
    m_contactsPage = new ContactsPage(m_app ? m_app->beaconService() : nullptr, this);
    m_transferPage = new TransferCenterPage(this);
    m_sharePage    = new SharePage(this);
    m_settingsPage = new SettingsPage(this);

    // 向 ChatPage 注入依赖：SignalService 和本机 peer_id
    if (m_app) {
        m_chatPage->setSignalService(m_app->signalService());
        m_chatPage->setLocalPeerId(m_app->settings()->peerId());
    }

    // 双击联系人 → 打开聊天会话并切换到聊天页
    connect(m_contactsPage, &ContactsPage::peerActivated, this, [this](const PeerInfo& peer) {
        m_chatPage->openConversation(peer);  // 打开具体会话
        switchToPage(0);                      // 切换到聊天页
    });

    // 添加页面到栈，索引与 kNavItems 中 pageIndex 一致
    m_pageStack->addWidget(m_chatPage);      // 0
    m_pageStack->addWidget(m_contactsPage);  // 1
    m_pageStack->addWidget(m_transferPage);  // 2
    m_pageStack->addWidget(m_sharePage);     // 3
    m_pageStack->addWidget(m_settingsPage);  // 4
}

} // namespace FengSui
