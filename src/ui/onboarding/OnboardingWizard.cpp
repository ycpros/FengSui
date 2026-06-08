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
    setWindowTitle(QString::fromUtf8("\xe7\x83\xbd\xe7\x87\xa7 FengSui - \xe9\xa6\x96\xe6\xac\xa1\xe8\xae\xbe\xe7\xbd\xae"));
    setMinimumSize(520, 400);
    resize(520, 400);

    // 向导按钮文字中文化
    setButtonText(QWizard::NextButton,    QString::fromUtf8("\xe4\xb8\x8b\xe4\xb8\x80\xe6\xad\xa5"));  // 下一步
    setButtonText(QWizard::BackButton,    QString::fromUtf8("\xe4\xb8\x8a\xe4\xb8\x80\xe6\xad\xa5"));  // 上一步
    setButtonText(QWizard::FinishButton,  QString::fromUtf8("\xe5\xae\x8c\xe6\x88\x90"));            // 完成
    setButtonText(QWizard::CancelButton,  QString::fromUtf8("\xe5\x8f\x96\xe6\xb6\x88"));            // 取消

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
    page->setTitle(QString::fromUtf8("\xe8\xae\xbe\xe7\xbd\xae\xe6\x82\xa8\xe7\x9a\x84\xe6\x98\xb5\xe7\xa7\xb0"));  // 设置您的昵称
    page->setSubTitle(QString::fromUtf8("\xe5\x85\xb6\xe4\xbb\x96\xe8\xae\xbe\xe5\xa4\x87\xe5\xb0\x86\xe9\x80\x9a\xe8\xbf\x87\xe6\xad\xa4\xe5\x90\x8d\xe7\xa7\xb0\xe8\xaf\x86\xe5\x88\xab\xe6\x82\xa8\xe3\x80\x82"));  // 其他设备将通过此名称识别您。

    auto* layout = new QVBoxLayout(page);

    auto* label = new QLabel(QString::fromUtf8("\xe6\x98\xb5\xe7\xa7\xb0:"), page);  // 昵称:
    m_nicknameEdit = new QLineEdit(page);
    m_nicknameEdit->setPlaceholderText(QString::fromUtf8("\xe8\xbe\x93\xe5\x85\xa5\xe6\x82\xa8\xe7\x9a\x84\xe6\x98\xb5\xe7\xa7\xb0"));  // 输入您的昵称

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
    page->setTitle(QString::fromUtf8("\xe7\xbd\x91\xe7\xbb\x9c\xe5\x8f\x91\xe7\x8e\xb0"));  // 网络发现
    page->setSubTitle(QString::fromUtf8(
        "\xe5\xbc\x80\xe5\x90\xaf\xe5\x90\x8e\xef\xbc\x8c\xe6\x82\xa8\xe5\x8f\xaf\xe4\xbb\xa5\xe8\x87\xaa\xe5\x8a\xa8\xe5\x8f\x91\xe7\x8e\xb0\xe5\xb1\x80\xe5\x9f\x9f\xe7\xbd\x91\xe5\x86\x85\xe5\x85\xb6\xe4\xbb\x96\xe8\xbf\x90\xe8\xa1\x8c\xe4\xb8\xad\xe7\x9a\x84\xe5\xb3\xb0\xe7\x87\xa7\xe5\xae\xa2\xe6\x88\xb7\xe7\xab\xaf\xe3\x80\x82\n"
        "\xe5\xbb\xba\xe8\xae\xae\xe4\xbf\x9d\xe6\x8c\x81\xe5\xbc\x80\xe5\x90\xaf\xe3\x80\x82"));
        // 开启后，您可以自动发现局域网内其他运行中的烽燧客户端。
        // 建议保持开启。

    auto* layout = new QVBoxLayout(page);

    m_discoveryCheck = new QCheckBox(
        QString::fromUtf8("\xe5\x90\xaf\xe7\x94\xa8\xe5\xb1\x80\xe5\x9f\x9f\xe7\xbd\x91\xe8\xae\xbe\xe5\xa4\x87\xe8\x87\xaa\xe5\x8a\xa8\xe5\x8f\x91\xe7\x8e\xb0"), page);  // 启用局域网设备自动发现
    m_discoveryCheck->setChecked(true);  // 默认开启

    layout->addStretch();
    layout->addWidget(m_discoveryCheck);
    layout->addStretch();

    return page;
}

QWizardPage* OnboardingWizard::createDownloadPage()
{
    auto* page = new QWizardPage(this);
    page->setTitle(QString::fromUtf8("\xe9\xbb\x98\xe8\xae\xa4\xe4\xb8\x8b\xe8\xbd\xbd\xe7\x9b\xae\xe5\xbd\x95"));  // 默认下载目录
    page->setSubTitle(QString::fromUtf8("\xe6\x8e\xa5\xe6\x94\xb6\xe7\x9a\x84\xe6\x96\x87\xe4\xbb\xb6\xe5\xb0\x86\xe4\xbf\x9d\xe5\xad\x98\xe5\x88\xb0\xe6\xad\xa4\xe7\x9b\xae\xe5\xbd\x95\xe3\x80\x82"));  // 接收的文件将保存到此目录。

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
        QString::fromUtf8("\xe6\xb5\x8f\xe8\xa7\x88..."), page);  // 浏览...
    connect(browseButton, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(
            this,
            QString::fromUtf8("\xe9\x80\x89\xe6\x8b\xa9\xe4\xb8\x8b\xe8\xbd\xbd\xe7\x9b\xae\xe5\xbd\x95"),  // 选择下载目录
            m_downloadPathEdit->text());
        if (!dir.isEmpty()) {
            m_downloadPathEdit->setText(dir);
        }
    });

    pathLayout->addWidget(m_downloadPathEdit, 1);
    pathLayout->addWidget(browseButton);

    layout->addStretch();
    layout->addWidget(new QLabel(QString::fromUtf8("\xe4\xb8\x8b\xe8\xbd\xbd\xe8\xb7\xaf\xe5\xbe\x84:"), page));  // 下载路径:
    layout->addLayout(pathLayout);
    layout->addStretch();

    return page;
}

} // namespace FengSui
