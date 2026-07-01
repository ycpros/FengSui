// OnboardingViewModel.h
// 首次启动向导 ViewModel：收集昵称、发现开关、下载目录、授权网卡，
// 完成时写入 AppSettings 并交由 AppController 启动服务。

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

namespace FengSui {

class Application;
class AppSettings;

class OnboardingViewModel : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString nickname READ nickname WRITE setNickname NOTIFY nicknameChanged)
    Q_PROPERTY(bool discoveryEnabled READ discoveryEnabled WRITE setDiscoveryEnabled NOTIFY discoveryEnabledChanged)
    Q_PROPERTY(QString downloadDir READ downloadDir WRITE setDownloadDir NOTIFY downloadDirChanged)
    // 候选网卡 [{id,name,cidr,physical,selected}...]
    Q_PROPERTY(QVariantList interfaces READ interfaces NOTIFY interfacesChanged)
    // 是否需要显示网卡选择步骤（多物理网卡时才需要）
    Q_PROPERTY(bool needsInterfaceStep READ needsInterfaceStep NOTIFY interfacesChanged)

public:
    explicit OnboardingViewModel(QObject* parent = nullptr);

    void bind(Application* app);

    QString nickname() const { return m_nickname; }
    void setNickname(const QString& v);

    bool discoveryEnabled() const { return m_discoveryEnabled; }
    void setDiscoveryEnabled(bool v);

    QString downloadDir() const { return m_downloadDir; }
    void setDownloadDir(const QString& v);

    QVariantList interfaces() const;
    bool needsInterfaceStep() const;

    Q_INVOKABLE void toggleInterface(const QString& id, bool selected);
    // 提交：写入设置并触发 AppController.completeOnboarding
    Q_INVOKABLE void finish();

signals:
    void nicknameChanged();
    void discoveryEnabledChanged();
    void downloadDirChanged();
    void interfacesChanged();
    void finished();

private:
    Application* m_app = nullptr;
    AppSettings* m_settings = nullptr;

    QString      m_nickname;
    bool         m_discoveryEnabled = true;
    QString      m_downloadDir;
    QStringList  m_selectedIds;
};

} // namespace FengSui
