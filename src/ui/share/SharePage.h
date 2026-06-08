// SharePage.h
// 共享文件页面：浏览在线共享 + 管理我的共享。
// 任务 005/013/014 中实现。

#pragma once

#include <QWidget>

namespace FengSui {

class SharePage : public QWidget {
    Q_OBJECT

public:
    explicit SharePage(QWidget* parent = nullptr);
};

} // namespace FengSui
