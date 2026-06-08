// TransferCenterPage.h
// 传输中心：展示所有文件传输任务的统一面板。
// 任务 005/012 中实现。

#pragma once

#include <QWidget>

namespace FengSui {

class TransferCenterPage : public QWidget {
    Q_OBJECT

public:
    explicit TransferCenterPage(QWidget* parent = nullptr);
};

} // namespace FengSui
