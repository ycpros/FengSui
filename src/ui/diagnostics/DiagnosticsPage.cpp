// DiagnosticsPage.cpp
// 网络诊断页面占位：本机 IP、端口、UDP 测试、防火墙检测。任务 017 中实现。

#include "ui/diagnostics/DiagnosticsPage.h"

#include <QLabel>
#include <QVBoxLayout>

namespace FengSui
{
    DiagnosticsPage::DiagnosticsPage(QWidget* parent)
        : QWidget(parent)
    {
        auto* layout = new QVBoxLayout(this);

        // 页面标题
        auto* titleLabel = new QLabel(QStringLiteral("网络诊断"), this);
        titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; padding: 12px;");

        // 空态提示
        auto* placeholderLabel = new QLabel(QStringLiteral("诊断功能即将完善"), this);
        placeholderLabel->setAlignment(Qt::AlignCenter);
        placeholderLabel->setStyleSheet("color: #999; font-size: 14px;");

        layout->addWidget(titleLabel);
        layout->addStretch();
        layout->addWidget(placeholderLabel);
        layout->addStretch();
    }
} // namespace FengSui
