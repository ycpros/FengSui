// SettingsPage.h
// 设置页面：常规设置、网络设置和诊断入口。

#pragma once

#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QSpinBox;

namespace FengSui {

class AppSettings;
class ManualPeerRepository;
class NetworkPolicy;

class SettingsPage : public QWidget {
    Q_OBJECT

public:
    explicit SettingsPage(AppSettings* settings = nullptr,
                          NetworkPolicy* networkPolicy = nullptr,
                          ManualPeerRepository* manualPeerRepository = nullptr,
                          QWidget* parent = nullptr);

private:
    void setupUi();
    QWidget* createGeneralTab();
    QWidget* createNetworkTab();
    void loadSettingsIntoControls();
    void rebuildInterfaceList();
    void refreshManualPeers();
    void showRestartHint();
    void persistInterfaceSelection();

    AppSettings* m_settings = nullptr;
    NetworkPolicy* m_networkPolicy = nullptr;
    ManualPeerRepository* m_manualPeerRepository = nullptr;

    bool m_loading = false;

    QLineEdit* m_displayNameEdit = nullptr;
    QLineEdit* m_downloadDirEdit = nullptr;
    QCheckBox* m_autoStartCheck = nullptr;
    QCheckBox* m_minimizeToTrayCheck = nullptr;
    QCheckBox* m_discoveryCheck = nullptr;
    QSpinBox* m_portSpin = nullptr;
    QComboBox* m_networkModeCombo = nullptr;
    QListWidget* m_interfaceList = nullptr;
    QPlainTextEdit* m_allowedCidrsEdit = nullptr;
    QListWidget* m_manualPeerList = nullptr;
    QLabel* m_restartHintLabel = nullptr;
};

} // namespace FengSui
