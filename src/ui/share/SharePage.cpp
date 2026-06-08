// SharePage.cpp
// 共享文件页面占位："浏览共享" + "我的共享" 两个标签页。后续任务 013/014 中实现。

#include "ui/share/SharePage.h"

#include <QLabel>
#include <QTabWidget>
#include <QVBoxLayout>

namespace FengSui {

SharePage::SharePage(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    // 页面标题
    auto* titleLabel = new QLabel(QStringLiteral("共享文件"), this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; padding: 12px;");

    // 两个标签页
    auto* tabWidget = new QTabWidget(this);

    // "浏览共享" 标签
    auto* browseTab = new QWidget();
    auto* browseLayout = new QVBoxLayout(browseTab);
    auto* browseEmpty = new QLabel(QStringLiteral("暂无在线共享"));
    browseEmpty->setAlignment(Qt::AlignCenter);
    browseEmpty->setStyleSheet("color: #999; font-size: 14px;");
    browseLayout->addStretch();
    browseLayout->addWidget(browseEmpty);
    browseLayout->addStretch();

    // "我的共享" 标签
    auto* myTab = new QWidget();
    auto* myLayout = new QVBoxLayout(myTab);
    auto* myEmpty = new QLabel(QStringLiteral("暂无共享目录"));
    myEmpty->setAlignment(Qt::AlignCenter);
    myEmpty->setStyleSheet("color: #999; font-size: 14px;");
    myLayout->addStretch();
    myLayout->addWidget(myEmpty);
    myLayout->addStretch();

    tabWidget->addTab(browseTab, QStringLiteral("浏览共享"));
    tabWidget->addTab(myTab, QStringLiteral("我的共享"));

    layout->addWidget(titleLabel);
    layout->addWidget(tabWidget);
}

} // namespace FengSui
