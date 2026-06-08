// TransferCenterPage.cpp
// 传输中心页面占位：展示所有文件传输任务。后续任务 012 中实现完整传输面板。

#include "ui/transfer_center/TransferCenterPage.h"

#include <QLabel>
#include <QVBoxLayout>

namespace FengSui {

TransferCenterPage::TransferCenterPage(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    // 页面标题
    auto* titleLabel = new QLabel(QStringLiteral("传输中心"), this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; padding: 12px;");

    // 空态提示
    auto* emptyLabel = new QLabel(QStringLiteral("暂无传输任务"), this);
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setStyleSheet("color: #999; font-size: 14px;");

    layout->addWidget(titleLabel);
    layout->addStretch();
    layout->addWidget(emptyLabel);
    layout->addStretch();
}

} // namespace FengSui
