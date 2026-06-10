// OnboardingWizard.h
// 首次启动向导：3 步设置（昵称 → 发现开关 → 下载目录）。
// 用户完成全部步骤后写 AppSettings，中途关闭不保存。

#pragma once

#include "platform/InterfaceEnumerator.h"

#include <QCheckBox>

#include <QWizard>

class QLineEdit;
class QCheckBox;

namespace FengSui {

class AppSettings;

// 首次启动向导页面定义
enum OnboardingPage {
    Page_Nickname  = 0,   // 第 1 步：设置昵称
    Page_Discovery = 1,   // 第 2 步：网络发现开关
    Page_Download   = 2,   // 第 3 步：默认下载目录
    Page_Interfaces = 3    // 第 4 步：选择授权网卡
};

// 向导类：3 个 QWizardPage，完成后写入 AppSettings。
// 外部通过 accepted() 信号得知用户完成，通过 rejected() 得知取消。
// 注意：QWizard 的 rejected() 在用户点击取消或关闭窗口时触发，此时不保存。
class OnboardingWizard : public QWizard {
    Q_OBJECT

public:
    explicit OnboardingWizard(AppSettings* settings, QWidget* parent = nullptr);

    // 重写 reject：关闭窗口不保存设置
    void reject() override;

    // 重写 accept：保存设置并标记向导完成
    void accept() override;

private:
    // 创建第 1 页：昵称输入
    QWizardPage* createNicknamePage();

    // 创建第 2 页：网络发现开关
    QWizardPage* createDiscoveryPage();

    // 创建第 4 页：选择授权网卡
    QWizardPage* createInterfacesPage();

    // 创建第 3 页：下载目录选择
    QWizardPage* createDownloadPage();

    AppSettings* m_settings = nullptr;

    // 各页面控件引用（用于 accept 时读取值）
    QLineEdit*  m_nicknameEdit = nullptr;
    QCheckBox*  m_discoveryCheck = nullptr;
    QLineEdit*  m_downloadPathEdit = nullptr;

    // 第 4 步控件
    QList<QCheckBox*>            m_ifaceChecks;   // 每个网卡的复选框
    QList<NetworkInterfaceInfo>  m_ifaceInfos;    // 对应的网卡信息快照
};

} // namespace FengSui
