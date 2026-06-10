// AppSettings.h
// 应用设置管理，封装 SQLite settings 表读写，提供类型安全的配置访问。
// 对应 05_data_model.md 的 AppSettings struct 定义。

#pragma once

#include <QObject>
#include <QString>

namespace FengSui {

class SettingsRepository;

// 应用设置管理器。
// 在 Application 中创建，全局唯一。所有设置变更即时持久化到 SQLite。
class AppSettings : public QObject {
    Q_OBJECT

public:
    explicit AppSettings(SettingsRepository* repository, QObject* parent = nullptr);

    // 从持久化存储加载设置，返回 true 表示加载成功
    bool load();

    // 保存所有设置到持久化存储
    void save();

    // ---- 常规设置 ----

    // 用户昵称（默认取系统主机名）
    QString displayName() const;
    void setDisplayName(const QString& name);

    // 默认下载目录（默认系统下载文件夹）
    QString downloadDir() const;
    void setDownloadDir(const QString& dir);

    // 开机自启
    bool autoStart() const;
    void setAutoStart(bool enabled);

    // 关闭后最小化到托盘
    bool minimizeToTray() const;
    void setMinimizeToTray(bool enabled);

    // ---- 网络设置 ----

    // 获取本机设备唯一标识。
    // 返回值：settings 表中的 peer_id；load() 会在缺失时生成并持久化。
    // 线程安全性：仅在主线程或同一 QObject 所属线程调用。
    QString peerId() const;

    // 写入本机设备唯一标识。
    // peerId: 设备唯一标识，不能为空；调用方负责保证其稳定性。
    // 线程安全性：仅在主线程或同一 QObject 所属线程调用。
    void setPeerId(const QString& peerId);

    // 是否启用局域网设备发现
    bool discoveryEnabled() const;
    void setDiscoveryEnabled(bool enabled);

    // TCP 消息服务监听端口（默认 8787）
    quint16 listenPort() const;
    void setListenPort(quint16 port);

    // ---- 网络策略 ----

    // 授权网卡 ID 列表（逗号分隔），默认空
    QString selectedInterfaces() const;
    void setSelectedInterfaces(const QString& ids);

    // 允许网段 CIDR 列表（逗号分隔），默认空
    QString allowedCidrs() const;
    void setAllowedCidrs(const QString& cidrs);

    // 网络模式：secure_lan / multi_lan / compat_test，默认 secure_lan
    QString networkMode() const;
    void setNetworkMode(const QString& mode);

    // ---- 向导状态 ----

    // 是否已完成首次启动向导
    bool onboardingCompleted() const;
    void setOnboardingCompleted(bool completed);

signals:
    // 设置变更通知，key 为空表示多项变更
    void settingChanged(const QString& key);

private:
    // 读取单个设置值
    QString readValue(const QString& key, const QString& defaultValue = QString()) const;

    // 写入单个设置值
    void writeValue(const QString& key, const QString& value);

    SettingsRepository* m_repository = nullptr;
};

} // namespace FengSui
