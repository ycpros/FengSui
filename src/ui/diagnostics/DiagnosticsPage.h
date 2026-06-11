// DiagnosticsPage.h
// 网络诊断页面：展示本机网络状态和未接入检测项。

#pragma once

#include <QWidget>

class QLabel;
class QTableWidget;

namespace FengSui {

class AppSettings;
class BeaconService;
class NetworkPolicy;

class DiagnosticsPage : public QWidget {
    Q_OBJECT

public:
    explicit DiagnosticsPage(AppSettings* settings = nullptr,
                             BeaconService* beaconService = nullptr,
                             NetworkPolicy* networkPolicy = nullptr,
                             QWidget* parent = nullptr);

private:
    void setupUi();
    void refresh();
    void renderInterfaces();
    void renderChecks();
    void setCheckRow(int row,
                     const QString& item,
                     const QString& status,
                     const QString& detail);

    AppSettings* m_settings = nullptr;
    BeaconService* m_beaconService = nullptr;
    NetworkPolicy* m_networkPolicy = nullptr;

    QLabel* m_modeLabel = nullptr;
    QLabel* m_portLabel = nullptr;
    QLabel* m_cidrsLabel = nullptr;
    QLabel* m_onlinePeersLabel = nullptr;
    QTableWidget* m_interfaceTable = nullptr;
    QTableWidget* m_checkTable = nullptr;
};

} // namespace FengSui
