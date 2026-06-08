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
        auto* titleLabel = new QLabel(QString::fromUtf8("\xe7\xbd\x91\xe7\xbb\x9c\xe8\xaf\x8a\xe6\x96\xad"), this);
        titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; padding: 12px;");

        // 空态提示
        auto* placeholderLabel = new QLabel(QString::fromUtf8(
            "\xe8\xaf\x8a\xe6\x96\xad\xe5\x8a\x9f\xe8\x83\xbd\xe5\x8d\xb3\xe5\xb0\x86\xe5\xae\x8c\xe5\x96\x84"), this);
        placeholderLabel->setAlignment(Qt::AlignCenter);
        placeholderLabel->setStyleSheet("color: #999; font-size: 14px;");

        layout->addWidget(titleLabel);
        layout->addStretch();
        layout->addWidget(placeholderLabel);
        layout->addStretch();
    }
} // namespace FengSui