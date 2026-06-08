// OnboardingWizard.cpp
// 首次启动向导实现：3 步设置流程。

#include "ui/onboarding/OnboardingWizard.h"
#include "app/AppSettings.h"

#include <QCheckBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QWizardPage>

namespace FengSui {

// 向导页面辅助类：将 QWizardPage 的 protected registerField 暴露为公开方法。
class RequiredFieldWizardPage : public QWizardPage {
public:
    using QWizardPage::registerField;  // protected -> public
};


OnboardingWizard::OnboardingWizard(AppSettings* settings, QWidget* parent)
    : QWizard(parent)
    , m_settings(settings)
{
    setWindowTitle(QStringLiteral("烽燧 FengSui - 首次设置"));
    setMinimumSize(520, 400);
    resize(520, 400);

    // 向导按钮文字中文化
    setButtonText(QWizard::NextButton, QStringLiteral("下一步"));
    setButtonText(QWizard::BackButton, QStringLiteral("上一步"));
    setButtonText(QWizard::FinishButton, QStringLiteral("完成"));
    setButtonText(QWizard::CancelButton, QStringLiteral("取消"));

    // 添加 3 个页面
    setPage(Page_Nickname,  createNicknamePage());
    setPage(Page_Discovery, createDiscoveryPage());
    setPage(Page_Download,  createDownloadPage());

    // 设置起始页
    setStartId(Page_Nickname);
}

void OnboardingWizard::reject()
{
    // 用户取消或关闭窗口：不保存任何设置，直接关闭
    QWizard::reject();
}

void OnboardingWizard::accept()
{
    // 用户点击"完成"：保存所有设置
    if (m_settings && m_nicknameEdit) {
        m_settings->setDisplayName(m_nicknameEdit->text().trimmed());
    }
    if (m_settings && m_discoveryCheck) {
        m_settings->setDiscoveryEnabled(m_discoveryCheck->isChecked());
    }
    if (m_settings && m_downloadPathEdit) {
        m_settings->setDownloadDir(m_downloadPathEdit->text());
    }
    if (m_settings) {
        m_settings->setOnboardingCompleted(true);
    }

    QWizard::accept();
}

QWizardPage* OnboardingWizard::createNicknamePage()
{
    auto* page = new RequiredFieldWizardPage();
    page->setTitle(QStringLiteral("设置您的昵称"));
    page->setSubTitle(QStringLiteral("其他设备将通过此名称识别您。"));

    auto* layout = new QVBoxLayout(page);

    auto* label = new QLabel(QStringLiteral("昵称:"), page);
    m_nicknameEdit = new QLineEdit(page);
    m_nicknameEdit->setPlaceholderText(QStringLiteral("输入您的昵称"));

    // 默认使用已设置的显示名称（通常为系统主机名）
    if (m_settings) {
        m_nicknameEdit->setText(m_settings->displayName());
    }

    layout->addStretch();
    layout->addWidget(label);
    layout->addWidget(m_nicknameEdit);
    layout->addStretch();

    // 注册必填字段：昵称不能为空
    page->registerField("nickname*", m_nicknameEdit);

    return page;
}

QWizardPage* OnboardingWizard::createDiscoveryPage()
{
    auto* page = new QWizardPage(this);
    page->setTitle(QStringLiteral("网络发现"));
    page->setSubTitle(QStringLiteral(
        "开启后，您可以自动发现局域网内其他运行中的烽燧客户端。\n"
        "建议保持开启。"));

    auto* layout = new QVBoxLayout(page);

    m_discoveryCheck = new QCheckBox(
        QStringLiteral("启用局域网设备自动发现"), page);
    m_discoveryCheck->setChecked(true);  // 默认开启

    layout->addStretch();
    layout->addWidget(m_discoveryCheck);
    layout->addStretch();

    return page;
}

QWizardPage* OnboardingWizard::createDownloadPage()
{
    auto* page = new QWizardPage(this);
    page->setTitle(QStringLiteral("默认下载目录"));
    page->setSubTitle(QStringLiteral("接收的文件将保存到此目录。"));

    auto* layout = new QVBoxLayout(page);

    auto* pathLayout = new QHBoxLayout();
    m_downloadPathEdit = new QLineEdit(page);
    m_downloadPathEdit->setReadOnly(true);

    // 默认使用系统下载目录
    QString defaultPath = m_settings
        ? m_settings->downloadDir()
        : QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    m_downloadPathEdit->setText(defaultPath);

    auto* browseButton = new QPushButton(
        QStringLiteral("浏览..."), page);
    connect(browseButton, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(
            this,
            QStringLiteral("选择下载目录"),
            m_downloadPathEdit->text());
        if (!dir.isEmpty()) {
            m_downloadPathEdit->setText(dir);
        }
    });

    pathLayout->addWidget(m_downloadPathEdit, 1);
    pathLayout->addWidget(browseButton);

    layout->addStretch();
    layout->addWidget(new QLabel(QStringLiteral("下载路径:"), page));
    layout->addLayout(pathLayout);
    layout->addStretch();

    return page;
}

} // namespace FengSui
