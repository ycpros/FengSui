// ShareAccessDialog.h
// Modal shell for future shared-folder access authorization.

#pragma once

#include <QDialog>

class QCheckBox;
class QLabel;

namespace FengSui {

class ShareAccessDialog : public QDialog {
    Q_OBJECT

public:
    explicit ShareAccessDialog(const QString& requesterName = QString(),
                               const QString& deviceName = QString(),
                               const QString& shareName = QString(),
                               QWidget* parent = nullptr);

    bool rememberChoice() const;
    void setRequestDetails(const QString& requesterName,
                           const QString& deviceName,
                           const QString& shareName);

private:
    QLabel* m_requesterLabel = nullptr;
    QLabel* m_shareLabel = nullptr;
    QCheckBox* m_rememberCheck = nullptr;
};

} // namespace FengSui
