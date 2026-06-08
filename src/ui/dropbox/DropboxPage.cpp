// DropboxPage.cpp
// 文件投递箱页面占位实现。P1 功能，后续完善。

#include "ui/dropbox/DropboxPage.h"

#include <QLabel>
#include <QVBoxLayout>

namespace FengSui {

DropboxPage::DropboxPage(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    auto* titleLabel = new QLabel(QStringLiteral("文件投递箱"), this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; padding: 12px;");

    auto* emptyLabel = new QLabel(QStringLiteral("暂无投递任务"), this);
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setStyleSheet("color: #999; font-size: 14px;");

    layout->addWidget(titleLabel);
    layout->addStretch();
    layout->addWidget(emptyLabel);
    layout->addStretch();
}

} // namespace FengSui
