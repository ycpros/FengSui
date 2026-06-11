// SharePage.h
// 共享文件页面：浏览在线共享 + 管理我的共享。
// 任务 005/013/014 中实现。

#pragma once

#include "models/SharedFolder.h"

#include <QList>
#include <QWidget>

class QLabel;
class QListWidget;

namespace FengSui {

class ShareService;

class SharePage : public QWidget {
    Q_OBJECT

public:
    explicit SharePage(QWidget* parent = nullptr);

    void setShareService(ShareService* service);

private:
    void renderMyShares();
    QWidget* createShareRow(const SharedFolder& folder);
    void showServiceMessage(const QString& message);

    ShareService* m_shareService = nullptr;
    QList<SharedFolder> m_sharedFolders;
    QListWidget* m_myShareList = nullptr;
    QLabel* m_serviceHint = nullptr;
};

} // namespace FengSui
