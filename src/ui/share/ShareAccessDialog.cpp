// ShareAccessDialog.cpp
// Authorization dialog UI for shared-folder access requests.

#include "ui/share/ShareAccessDialog.h"

#include "ui/UiStyle.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace FengSui {

ShareAccessDialog::ShareAccessDialog(const QString& requesterName,
                                     const QString& deviceName,
                                     const QString& shareName,
                                     QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("共享访问授权"));
    setModal(true);
    setMinimumWidth(420);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(22, 20, 22, 18);
    layout->setSpacing(14);

    auto* titleLabel = new QLabel(QStringLiteral("允许访问共享文件？"), this);
    titleLabel->setObjectName(QStringLiteral("shareAccessTitle"));
    titleLabel->setStyleSheet(UiStyle::pageTitleStyle());

    m_requesterLabel = new QLabel(this);
    m_requesterLabel->setObjectName(QStringLiteral("shareAccessRequesterLabel"));
    m_requesterLabel->setWordWrap(true);

    m_shareLabel = new QLabel(this);
    m_shareLabel->setObjectName(QStringLiteral("shareAccessShareLabel"));
    m_shareLabel->setWordWrap(true);
    m_shareLabel->setStyleSheet(UiStyle::mutedTextStyle());

    m_rememberCheck = new QCheckBox(QStringLiteral("记住我的选择"), this);
    m_rememberCheck->setObjectName(QStringLiteral("shareAccessRememberCheck"));

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* denyButton = new QPushButton(QStringLiteral("拒绝"), this);
    denyButton->setObjectName(QStringLiteral("denyAccessButton"));
    denyButton->setStyleSheet(UiStyle::secondaryButtonStyle());

    auto* allowButton = new QPushButton(QStringLiteral("允许"), this);
    allowButton->setObjectName(QStringLiteral("allowAccessButton"));
    allowButton->setProperty("primary", true);
    allowButton->setStyleSheet(UiStyle::primaryButtonStyle());

    buttonLayout->addWidget(denyButton);
    buttonLayout->addWidget(allowButton);

    layout->addWidget(titleLabel);
    layout->addWidget(m_requesterLabel);
    layout->addWidget(m_shareLabel);
    layout->addWidget(m_rememberCheck);
    layout->addLayout(buttonLayout);

    connect(denyButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(allowButton, &QPushButton::clicked, this, &QDialog::accept);

    setRequestDetails(requesterName, deviceName, shareName);
}

bool ShareAccessDialog::rememberChoice() const
{
    return m_rememberCheck && m_rememberCheck->isChecked();
}

void ShareAccessDialog::setRequestDetails(const QString& requesterName,
                                          const QString& deviceName,
                                          const QString& shareName)
{
    const QString requester = requesterName.trimmed().isEmpty()
        ? QStringLiteral("未知设备")
        : requesterName.trimmed();
    const QString device = deviceName.trimmed().isEmpty()
        ? QStringLiteral("未命名设备")
        : deviceName.trimmed();
    const QString share = shareName.trimmed().isEmpty()
        ? QStringLiteral("共享目录")
        : shareName.trimmed();

    if (m_requesterLabel) {
        m_requesterLabel->setText(
            QStringLiteral("%1（%2）请求访问您的共享文件。").arg(requester, device));
    }
    if (m_shareLabel) {
        m_shareLabel->setText(QStringLiteral("目标共享：%1").arg(share));
    }
}

} // namespace FengSui
