// DiagnosticsPage.h
// 网络诊断页面：本机 IP、端口、UDP 测试、防火墙检测。
// 任务 017 中实现。

#pragma once

#include <QWidget>

namespace FengSui {

class DiagnosticsPage : public QWidget {
    Q_OBJECT

public:
    explicit DiagnosticsPage(QWidget* parent = nullptr);
};

} // namespace FengSui
