// TrayManager.h
// 系统托盘管理：托盘图标、右键菜单、在线状态切换。
// P1 功能，后续实现。

#pragma once

#include <QObject>

namespace FengSui {

class TrayManager : public QObject {
    Q_OBJECT

public:
    explicit TrayManager(QObject* parent = nullptr);
};

} // namespace FengSui
