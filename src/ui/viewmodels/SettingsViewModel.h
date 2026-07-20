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
    // 根据当前勾选和实时网卡快照计算的预期 CIDR，只读且不读取持久化缓存。
    Q_PROPERTY(QString allowedCidrs READ allowedCidrs NOTIFY allowedCidrsChanged)

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
    // 返回当前授权选择在下次启动时预计生成的 CIDR，多个网段以逗号和空格分隔。
    // 返回值：当前没有物理候选网卡时返回空字符串。
    QString allowedCidrs() const;

    QVariantList interfaces() const;

    QString diagMode() const;
    int diagPort() const;
    QString diagAllowedCidrs() const;
    int diagOnlineCount() const;

    bool needsRestart() const { return m_needsRestart; }

    // 切换指定网卡的授权状态并写回 selectedInterfaces。
    // id: 网卡唯一 ID，必须来自 interfaces 属性提供的当前快照。
    // selected: true 表示授权，false 表示取消授权。
    // 取消最后一个有效选择时会按照自动策略回退到主物理网卡。该方法只刷新预期
    // CIDR 和重启提示，不会热重绑已运行的网络服务。
    Q_INVOKABLE void toggleInterface(const QString& id, bool selected);

    // 重新枚举网卡并通知 QML 刷新网卡、预期 CIDR 和运行时诊断摘要。
    // 该方法不修改当前 NetworkPolicy；网络服务仍需重启应用后使用新网卡状态。
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

    // 返回设置中原始的逗号分隔授权网卡 ID，不校验网卡当前是否存在。
    QStringList selectedInterfaceIds() const;

    // 将原始授权 ID 与当前物理候选网卡校正；全部失效时返回主物理网卡 ID。
    QStringList resolvedSelectedInterfaceIds() const;

    Application*  m_app = nullptr;
    AppSettings*  m_settings = nullptr;
    bool          m_needsRestart = false;
};

} // namespace FengSui
