// SettingsPage.h
// 设置页面：常规设置（昵称、下载目录）+ 网络设置（发现、端口）。
// 任务 005/016 中实现。

#pragma once

#include <QWidget>

namespace FengSui {

class SettingsPage : public QWidget {
    Q_OBJECT

public:
    explicit SettingsPage(QWidget* parent = nullptr);
};

} // namespace FengSui
