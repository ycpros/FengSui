// Application.h
// FengSui 应用主类，继承 QApplication，持有全局数据库、设置仓库和配置实例。
// 负责应用级初始化（日志、数据库连接、设置加载）和生命周期管理。

#pragma once

#include <QApplication>

namespace FengSui {

// 前向声明，避免在头文件中引入具体实现
class AppSettings;
class BeaconService;
class ConversationRepository;
class CourierService;
class Database;
class MessageRepository;
class SettingsRepository;
class SignalService;
class TransferRepository;

// FengSui 应用主类。
// 在 main() 中创建，负责初始化顺序和全局资源管理。
class Application : public QApplication {
    Q_OBJECT

public:
    Application(int& argc, char** argv);
    ~Application() override;

    // 初始化应用子系统（日志 → 数据库 → 设置仓库 → 设置），返回 true 表示成功
    bool initialize();

    // 启动局域网发现服务。
    // 返回值：启动成功返回 true；发现功能被配置关闭时也返回 true。
    // 线程安全性：仅在主线程调用。
    bool startBeaconService();

    // 停止局域网发现服务。
    // 退出应用前调用，内部会尽力发送离线广播。
    // 线程安全性：仅在主线程调用。
    void stopBeaconService();

    // 启动 TCP 消息服务。
    // 返回值：启动成功返回 true。
    // 线程安全性：仅在主线程调用。
    bool startSignalService();

    // 停止 TCP 消息服务。
    // 关闭所有活跃 TCP 连接和监听端口。
    // 线程安全性：仅在主线程调用。
    void stopSignalService();

    // 启动文件传输服务。
    // 应在 BeaconService 和 SignalService 均启动后调用。
    // 线程安全性：仅在主线程调用。
    void startCourierService();

    // 停止文件传输服务。
    // 取消所有进行中的传输任务。
    // 线程安全性：仅在主线程调用。
    void stopCourierService();

    // 获取全局设置实例
    AppSettings* settings() const;

    // 获取全局数据库实例
    Database* database() const;

    // 获取局域网发现服务实例。
    // 返回值：应用初始化成功后返回 BeaconService 指针，否则返回 nullptr。
    // 线程安全性：仅在主线程调用。
    BeaconService* beaconService() const;

    // 获取 TCP 消息服务实例。
    // 返回值：应用初始化成功后返回 SignalService 指针，否则返回 nullptr。
    // 线程安全性：仅在主线程调用。
    SignalService* signalService() const;

    // 获取会话存储仓库实例。
    ConversationRepository* conversationRepository() const;

    // 获取消息存储仓库实例。
    MessageRepository* messageRepository() const;

    // 获取文件传输编排服务实例。
    CourierService* courierService() const;

    // 获取传输任务存储仓库实例。
    TransferRepository* transferRepository() const;

private:
    AppSettings*         m_settings = nullptr;
    BeaconService*       m_beaconService = nullptr;
    SignalService*       m_signalService = nullptr;
    CourierService*      m_courierService = nullptr;
    Database*            m_database = nullptr;
    SettingsRepository*  m_settingsRepository = nullptr;
    ConversationRepository* m_conversationRepo = nullptr;
    MessageRepository*      m_messageRepo = nullptr;
    TransferRepository*     m_transferRepo = nullptr;
};

} // namespace FengSui
