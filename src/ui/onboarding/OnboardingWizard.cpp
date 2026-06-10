// OnboardingWizard.cpp
// 首次启动向导实现：3 步设置流程。

#include "ui/onboarding/OnboardingWizard.h"
#include "platform/InterfaceEnumerator.h"
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

    // 添加基础页面。唯一候选物理网卡环境自动跳过网卡选择页。
    setPage(Page_Nickname,  createNicknamePage());
    setPage(Page_Discovery, createDiscoveryPage());
    setPage(Page_Download,   createDownloadPage());
    if (InterfaceEnumerator::candidates().size() != 1) {
        setPage(Page_Interfaces, createInterfacesPage());
    }

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
    // 网络策略：收集选中的授权网卡及对应 CIDR
    QStringList selectedIds;
    QStringList selectedCidrs;
    for (int i = 0; i < m_ifaceChecks.size(); ++i) {
        if (m_ifaceChecks[i]->isChecked() && m_ifaceChecks[i]->isEnabled()
            && i < m_ifaceInfos.size()) {
            selectedIds.append(m_ifaceInfos[i].id);
            selectedCidrs.append(m_ifaceInfos[i].cidr());
        }
    }
    // 单网卡环境（无控件列表），自动取第一个候选网卡
    if (selectedIds.isEmpty()) {
        const auto candidates = InterfaceEnumerator::candidates();
        if (!candidates.isEmpty()) {
            selectedIds.append(candidates.first().id);
            selectedCidrs.append(candidates.first().cidr());
        }
    }
    if (m_settings) {
        m_settings->setSelectedInterfaces(selectedIds.join(QStringLiteral(",")));
        m_settings->setAllowedCidrs(selectedCidrs.join(QStringLiteral(",")));
        m_settings->setNetworkMode(QStringLiteral("secure_lan"));
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

QWizardPage* OnboardingWizard::createInterfacesPage()
{
    auto* page = new QWizardPage(this);
    page->setTitle(QStringLiteral("选择授权网卡"));
    page->setSubTitle(QStringLiteral("选择烽燧允许使用的内网网卡。虚拟网卡和公网地址默认不启用。"));

    auto* layout = new QVBoxLayout(page);

    // 获取全量接口信息
    const QList<NetworkInterfaceInfo> allIfaces = InterfaceEnumerator::enumerate();

    // 统计物理网卡数量，0 个时给出诊断提示
    int candidateCount = 0;
    for (const auto& iface : allIfaces) {
        if (iface.type == InterfaceType::Physical)
            candidateCount++;
    }

    bool hasCheckedPhysical = false;

    // 按类型展示每个网卡（排除回环）
    for (const NetworkInterfaceInfo& iface : allIfaces) {
        if (iface.type == InterfaceType::Loopback) continue;

        QString reason;
        bool    enabled = false;
        // 第一个物理网卡默认勾选；单网卡场景也勾选
        bool    checked = (iface.type == InterfaceType::Physical)
                       && (candidateCount == 1 || !hasCheckedPhysical);

        switch (iface.type) {
        case InterfaceType::Physical:
            enabled = true;
            if (checked) {
                hasCheckedPhysical = true;
            }
            break;
        case InterfaceType::Virtual:
            reason  = QStringLiteral(" — 虚拟网卡");
            enabled = false;
            break;
        case InterfaceType::LinkLocal:
            reason  = QStringLiteral(" — 链路本地地址 169.254.x.x");
            enabled = false;
            break;
        case InterfaceType::Public:
            reason  = QStringLiteral(" — 公网地址，不建议内网使用");
            enabled = false;
            break;
        default:
            reason  = QStringLiteral(" — 状态未知");
            enabled = false;
            break;
        }

        auto* check = new QCheckBox(iface.displayName + reason, page);
        check->setEnabled(enabled);
        check->setChecked(checked);

        // secure_lan 单选行为：选中新项时取消其他已勾选的启用项
        if (enabled) {
            connect(check, &QCheckBox::toggled, this,
                    [this, check](bool on) {
                        if (!on) return;
                        for (QCheckBox* other : m_ifaceChecks) {
                            if (other != check && other->isEnabled())
                                other->setChecked(false);
                        }
                    });
        }

        layout->addWidget(check);
        m_ifaceChecks.append(check);
        m_ifaceInfos.append(iface);
    }

    // 单网卡或无网卡环境的提示
    if (candidateCount <= 1) {
        auto* infoLabel = new QLabel(page);
        if (candidateCount == 1) {
            infoLabel->setText(QStringLiteral("已自动选择唯一的物理网卡。"));
        } else {
            infoLabel->setText(QStringLiteral("未检测到物理网卡，请检查网络连接后重试。"));
        }
        infoLabel->setStyleSheet("color: #666; font-size: 12px; padding-top: 8px;");
        layout->addWidget(infoLabel);
    }

    layout->addStretch();
    return page;
}

} // namespace FengSui
