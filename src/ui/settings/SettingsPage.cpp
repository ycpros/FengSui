// SettingsPage.cpp
// 设置页面占位：常规设置 + 网络设置。后续任务 016 中实现完整设置表单。

#include "ui/settings/SettingsPage.h"

#include <QLabel>
#include <QVBoxLayout>

namespace FengSui {

SettingsPage::SettingsPage(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    // 页面标题
    auto* titleLabel = new QLabel(QString::fromUtf8("\xe8\xae\xbe\xe7\xbd\xae"), this);  // 设置
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; padding: 12px;");

    // 占位内容
    auto* placeholderLabel = new QLabel(QString::fromUtf8("\xe8\xae\xbe\xe7\xbd\xae\xe9\xa1\xb5\xe9\x9d\xa2\xe5\x8d\xb3\xe5\xb0\x86\xe5\xae\x8c\xe5\x96\x84"), this);  // 设置页面即将完善
    placeholderLabel->setAlignment(Qt::AlignCenter);
    placeholderLabel->setStyleSheet("color: #999; font-size: 14px;");

    layout->addWidget(titleLabel);
    layout->addStretch();
    layout->addWidget(placeholderLabel);
    layout->addStretch();
}

} // namespace FengSui
