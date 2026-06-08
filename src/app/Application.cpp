// Application.cpp
// FengSui 应用主类的实现：初始化日志、连接数据库、加载设置。

#include "app/Application.h"
#include "app/AppSettings.h"
#include "app/Logger.h"
#include "core/BeaconService.h"
#include "core/CourierService.h"
#include "core/SignalService.h"
#include "storage/ConversationRepository.h"
#include "storage/Database.h"
#include "storage/MessageRepository.h"
#include "storage/SettingsRepository.h"
#include "storage/TransferRepository.h"

#include <QMetaType>

namespace FengSui {

Application::Application(int& argc, char** argv)
    : QApplication(argc, argv)
{
    // 设置应用元信息，影响 QStandardPaths 和窗口标题等
    setApplicationName("FengSui");
    setApplicationVersion("0.1.0");
    setOrganizationName("FengSui");
}

Application::~Application()
{
    // 按依赖逆序停止服务：先停传输（依赖消息通道），再停消息，最后停发现。
    stopCourierService();
    stopSignalService();
    stopBeaconService();

    delete m_courierService;
    m_courierService = nullptr;

    delete m_signalService;
    m_signalService = nullptr;

    delete m_beaconService;
    m_beaconService = nullptr;

    delete m_transferRepo;
    m_transferRepo = nullptr;

    delete m_settings;
    m_settings = nullptr;

    delete m_settingsRepository;
    m_settingsRepository = nullptr;

    delete m_database;
    m_database = nullptr;
}

bool Application::initialize()
{
    // 1. 初始化日志系统，确保后续所有 qDebug/qInfo/qWarning/qCritical 有输出
    Logger::instance().initialize();

    qInfo() << "FengSui v" << applicationVersion() << "starting...";

    // 2. 初始化数据库（建库建表）
    m_database = new Database(this);
    if (!m_database->initialize()) {
        qCritical() << "Failed to initialize database";
        return false;
    }

    // 3. 创建设置仓库，AppSettings 统一通过 SQLite settings 表读写
    m_settingsRepository = new SettingsRepository(m_database, this);

    // 4. 加载应用设置
    m_settings = new AppSettings(m_settingsRepository, this);
    if (!m_settings->load()) {
        qWarning() << "Failed to load settings, using defaults";
    }

    // 5. 创建局域网发现服务。是否启动由 main() 在首次向导完成后决定。
    m_beaconService = new BeaconService(m_settings, this);

    // 6. 创建 TCP 消息服务。是否启动由 main() 在首次向导完成后决定。
    m_signalService = new SignalService(m_settings, this);

    // 7. 创建消息存储仓库，注入到 SignalService 以实现消息持久化
    m_conversationRepo = new ConversationRepository(m_database,
                                                     m_settings->peerId(),
                                                     this);
    m_messageRepo = new MessageRepository(m_database, this);
    m_signalService->setConversationRepository(m_conversationRepo);
    m_signalService->setMessageRepository(m_messageRepo);

    // 8. 创建传输任务存储仓库
    m_transferRepo = new TransferRepository(m_database, this);

    // 9. 创建文件传输编排服务，注入依赖
    m_courierService = new CourierService(this);
    m_courierService->setSignalService(m_signalService);
    m_courierService->setTransferRepository(m_transferRepo);
    m_courierService->setLocalPeerId(m_settings->peerId());
    m_signalService->setCourierService(m_courierService);

    // 10. 注册元类型，使模型结构体可通过 Qt signal/slot 跨线程传递
    qRegisterMetaType<FengSui::Message>("FengSui::Message");
    qRegisterMetaType<FengSui::Conversation>("FengSui::Conversation");
    qRegisterMetaType<FengSui::TransferTask>("FengSui::TransferTask");

    qInfo() << "Application initialized successfully";
    return true;
}

bool Application::startBeaconService()
{
    if (!m_beaconService) {
        qWarning() << "Beacon service is not initialized";
        return false;
    }

    QString error;
    // BeaconService 内部会处理发现开关，因此这里不重复读取配置。
    if (!m_beaconService->start(error)) {
        qWarning() << "Failed to start beacon service:" << error;
        return false;
    }

    return true;
}

void Application::stopBeaconService()
{
    if (m_beaconService) {
        m_beaconService->stop();
    }
}

AppSettings* Application::settings() const
{
    return m_settings;
}

Database* Application::database() const
{
    return m_database;
}

BeaconService* Application::beaconService() const
{
    return m_beaconService;
}

bool Application::startSignalService()
{
    if (!m_signalService) {
        qWarning() << "Signal service is not initialized";
        return false;
    }

    QString error;
    if (!m_signalService->start(error)) {
        qWarning() << "Failed to start signal service:" << error;
        return false;
    }

    return true;
}

void Application::stopSignalService()
{
    if (m_signalService) {
        m_signalService->stop();
    }
}

SignalService* Application::signalService() const
{
    return m_signalService;
}

ConversationRepository* Application::conversationRepository() const
{
    return m_conversationRepo;
}

MessageRepository* Application::messageRepository() const
{
    return m_messageRepo;
}

void Application::startCourierService()
{
    // CourierService 是无状态启动（依赖 SignalService 的连接管理）。
    // 启动只需确保 SignalService 已在运行。
    if (!m_signalService || !m_signalService->isRunning()) {
        qWarning() << "SignalService must be running before starting CourierService";
        return;
    }
    qInfo() << "Courier service ready";
}

void Application::stopCourierService()
{
    // CourierService 的清理在析构函数中通过 delete 触发 ~CourierService()
    qInfo() << "Courier service stopping";
}

CourierService* Application::courierService() const
{
    return m_courierService;
}

TransferRepository* Application::transferRepository() const
{
    return m_transferRepo;
}

} // namespace FengSui
