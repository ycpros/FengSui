// SettingsViewModel.h
// 设置页 ViewModel：把 AppSettings 各项以 Q_PROPERTY 暴露给 QML，即时读写持久化；
// 并提供网卡列表与网络诊断摘要（只读）。主题模式复用 ThemeController。

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

namespace FengSui {

class Application;
class AppSettings;

class SettingsViewModel : public QObject {
    Q_OBJECT

    // 常规
    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName NOTIFY displayNameChanged)
    Q_PROPERTY(QString downloadDir READ downloadDir WRITE setDownloadDir NOTIFY downloadDirChanged)
    Q_PROPERTY(bool autoStart READ autoStart WRITE setAutoStart NOTIFY autoStartChanged)
    Q_PROPERTY(bool minimizeToTray READ minimizeToTray WRITE setMinimizeToTray NOTIFY minimizeToTrayChanged)

    // 网络
    Q_PROPERTY(bool discoveryEnabled READ discoveryEnabled WRITE setDiscoveryEnabled NOTIFY discoveryEnabledChanged)
    Q_PROPERTY(int listenPort READ listenPort WRITE setListenPort NOTIFY listenPortChanged)
    Q_PROPERTY(QString networkMode READ networkMode WRITE setNetworkMode NOTIFY networkModeChanged)
    Q_PROPERTY(QString allowedCidrs READ allowedCidrs WRITE setAllowedCidrs NOTIFY allowedCidrsChanged)

    // 网卡列表（[{id,name,displayName,cidr,type,physical,selected}...]）供 QML Repeater
    Q_PROPERTY(QVariantList interfaces READ interfaces NOTIFY interfacesChanged)

    // 诊断摘要（只读）
    Q_PROPERTY(QString diagMode READ diagMode NOTIFY diagChanged)
    Q_PROPERTY(int diagPort READ diagPort NOTIFY diagChanged)
    Q_PROPERTY(QString diagAllowedCidrs READ diagAllowedCidrs NOTIFY diagChanged)
    Q_PROPERTY(int diagOnlineCount READ diagOnlineCount NOTIFY diagChanged)

    // 网络设置变更后是否需要重启生效
    Q_PROPERTY(bool needsRestart READ needsRestart NOTIFY needsRestartChanged)

public:
    explicit SettingsViewModel(QObject* parent = nullptr);

    void bind(Application* app);

    QString displayName() const;      void setDisplayName(const QString& v);
    QString downloadDir() const;      void setDownloadDir(const QString& v);
    bool autoStart() const;           void setAutoStart(bool v);
    bool minimizeToTray() const;      void setMinimizeToTray(bool v);
    bool discoveryEnabled() const;    void setDiscoveryEnabled(bool v);
    int listenPort() const;           void setListenPort(int v);
    QString networkMode() const;      void setNetworkMode(const QString& v);
    QString allowedCidrs() const;     void setAllowedCidrs(const QString& v);

    QVariantList interfaces() const;

    QString diagMode() const;
    int diagPort() const;
    QString diagAllowedCidrs() const;
    int diagOnlineCount() const;

    bool needsRestart() const { return m_needsRestart; }

    // 切换某网卡的授权勾选（写回 selectedInterfaces）
    Q_INVOKABLE void toggleInterface(const QString& id, bool selected);
    // 重新枚举网卡与刷新诊断
    Q_INVOKABLE void refresh();

signals:
    void displayNameChanged();
    void downloadDirChanged();
    void autoStartChanged();
    void minimizeToTrayChanged();
    void discoveryEnabledChanged();
    void listenPortChanged();
    void networkModeChanged();
    void allowedCidrsChanged();
    void interfacesChanged();
    void diagChanged();
    void needsRestartChanged();

private:
    void markNeedsRestart();
    QStringList selectedInterfaceIds() const;

    Application*  m_app = nullptr;
    AppSettings*  m_settings = nullptr;
    bool          m_needsRestart = false;
};

} // namespace FengSui
