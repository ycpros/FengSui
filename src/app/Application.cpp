// Application.cpp
// FengSui 应用主类的实现：初始化日志、连接数据库、加载设置。

#include "app/Application.h"
#include "app/AppSettings.h"
#include "app/Logger.h"
#include "Version.h"
#include "core/BeaconService.h"
#include "core/CourierService.h"
#include "core/ShareService.h"
#include "core/SignalService.h"
#include "storage/ConversationRepository.h"
#include "storage/AccessGrantRepository.h"
#include "storage/Database.h"
#include "storage/DownloadLogRepository.h"
#include "storage/ManualPeerRepository.h"
#include "storage/MessageRepository.h"
#include "storage/SettingsRepository.h"
#include "storage/ShareRepository.h"
#include "storage/TransferRepository.h"
#include "models/NetworkPolicy.h"
#include "platform/InterfaceEnumerator.h"

#include <QMetaType>
#include <QJsonObject>
#include <QSet>
#include <QStringList>

namespace FengSui {

namespace {

QStringList splitCsvSetting(const QString& value)
{
    QStringList result;
    const QStringList parts = value.split(QStringLiteral(","),
                                          Qt::SkipEmptyParts);
    for (const QString& part : parts) {
        const QString trimmed = part.trimmed();
        if (!trimmed.isEmpty() && !result.contains(trimmed)) {
            result.append(trimmed);
        }
    }
    return result;
}

QSet<QString> toSet(const QStringList& values)
{
    QSet<QString> result;
    for (const QString& value : values) {
        if (!value.trimmed().isEmpty()) {
            result.insert(value.trimmed());
        }
    }
    return result;
}

QStringList cidrsForSelectedInterfaces(const QStringList& selectedIds)
{
    QStringList cidrs;
    const QSet<QString> selected = toSet(selectedIds);
    if (selected.isEmpty()) {
        return cidrs;
    }

    const QList<NetworkInterfaceInfo> interfaces = InterfaceEnumerator::enumerate();
    for (const NetworkInterfaceInfo& iface : interfaces) {
        if (!selected.contains(iface.id)) {
            continue;
        }
        const QString cidr = iface.cidr();
        if (NetworkPolicy::isValidCidr(cidr) && !cidrs.contains(cidr)) {
            cidrs.append(cidr);
        }
    }
    return cidrs;
}

QStringList validCidrs(const QStringList& cidrs)
{
    QStringList result;
    for (const QString& cidr : cidrs) {
        const QString trimmed = cidr.trimmed();
        if (NetworkPolicy::isValidCidr(trimmed) && !result.contains(trimmed)) {
            result.append(trimmed);
        }
    }
    return result;
}

} // namespace

Application::Application(int& argc, char** argv)
    : QApplication(argc, argv)
{
    // 设置应用元信息，影响 QStandardPaths 和窗口标题等
    setApplicationName("FengSui");
    setApplicationVersion(FENGSUI_VERSION_STRING);
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

    delete m_shareService;
    m_shareService = nullptr;

    delete m_transferRepo;
    m_transferRepo = nullptr;

    delete m_shareRepo;
    m_shareRepo = nullptr;

    delete m_downloadLogRepo;
    m_downloadLogRepo = nullptr;

    delete m_accessGrantRepo;
    m_accessGrantRepo = nullptr;

    delete m_manualPeerRepo;
    m_manualPeerRepo = nullptr;

    delete m_settings;
    m_settings = nullptr;

    delete m_settingsRepository;
    m_settingsRepository = nullptr;

    delete m_database;
    m_database = nullptr;

    delete m_networkPolicy;
    m_networkPolicy = nullptr;
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

    // 5. 构建运行时网络策略。首次无配置时会选择主物理网卡。
    m_networkPolicy = new NetworkPolicy();
    reloadNetworkPolicyFromSettings();

    // 6. 创建共享目录仓库与服务，供发现广播和共享页面使用。
    m_shareRepo = new ShareRepository(m_database, this);
    m_accessGrantRepo = new AccessGrantRepository(m_database, this);
    m_downloadLogRepo = new DownloadLogRepository(m_database, this);
    m_shareService = new ShareService(this);
    m_shareService->setShareRepository(m_shareRepo);
    m_shareService->setAppSettings(m_settings);
    m_shareService->setAccessGrantRepository(m_accessGrantRepo);
    m_shareService->setDownloadLogRepository(m_downloadLogRepo);

    // 7. 创建局域网发现服务。是否启动由 main() 在首次向导完成后决定。
    m_beaconService = new BeaconService(m_settings, m_networkPolicy, this);
    m_beaconService->setShareService(m_shareService);

    // 8. 创建 TCP 消息服务。是否启动由 main() 在首次向导完成后决定。
    m_signalService = new SignalService(m_settings, m_networkPolicy, this);
    m_signalService->setShareService(m_shareService);
    m_shareService->setSignalService(m_signalService);
    connect(m_shareService,
            &ShareService::outboundShareMessage,
            m_signalService,
            [this](const PeerInfo& peer, const QJsonObject& message) {
                if (m_signalService) {
                    m_signalService->sendJsonMessage(peer, message);
                }
            });

    // 9. 创建消息存储仓库，注入到 SignalService 以实现消息持久化
    m_conversationRepo = new ConversationRepository(m_database,
                                                     m_settings->peerId(),
                                                     this);
    m_messageRepo = new MessageRepository(m_database, this);
    m_signalService->setConversationRepository(m_conversationRepo);
    m_signalService->setMessageRepository(m_messageRepo);

    // 10. 创建手动添加设备仓库和传输任务存储仓库
    m_manualPeerRepo = new ManualPeerRepository(m_database, this);
    m_transferRepo = new TransferRepository(m_database, this);

    // 11. 创建文件传输编排服务，注入依赖
    m_courierService = new CourierService(this);
    m_courierService->setSignalService(m_signalService);
    m_courierService->setAppSettings(m_settings);
    m_courierService->setTransferRepository(m_transferRepo);
    m_courierService->setLocalPeerId(m_settings->peerId());
    m_signalService->setCourierService(m_courierService);

    // 12. 注册元类型，使模型结构体可通过 Qt signal/slot 跨线程传递
    qRegisterMetaType<FengSui::Message>("FengSui::Message");
    qRegisterMetaType<FengSui::Conversation>("FengSui::Conversation");
    qRegisterMetaType<FengSui::TransferTask>("FengSui::TransferTask");
    qRegisterMetaType<FengSui::RemoteSharedFolder>("FengSui::RemoteSharedFolder");
    qRegisterMetaType<FengSui::RemoteShareItem>("FengSui::RemoteShareItem");

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

ShareService* Application::shareService() const
{
    return m_shareService;
}

NetworkPolicy* Application::networkPolicy() const
{
    return m_networkPolicy;
}

void Application::reloadNetworkPolicyFromSettings()
{
    if (!m_networkPolicy) {
        m_networkPolicy = new NetworkPolicy();
    }
    if (!m_settings) {
        return;
    }

    const NetworkMode mode =
        NetworkPolicy::modeFromString(m_settings->networkMode());
    QStringList selectedIds = splitCsvSetting(m_settings->selectedInterfaces());
    QStringList cidrs = validCidrs(splitCsvSetting(m_settings->allowedCidrs()));

    bool shouldPersist = false;
    const QList<NetworkInterfaceInfo> candidates = InterfaceEnumerator::candidates();

    // 首次启动或旧配置为空时，默认选择主物理候选网卡。
    if (selectedIds.isEmpty() && !candidates.isEmpty()) {
        selectedIds.append(candidates.first().id);
        shouldPersist = true;
    }

    if (cidrs.isEmpty()) {
        cidrs = cidrsForSelectedInterfaces(selectedIds);
        if (cidrs.isEmpty() && !candidates.isEmpty()) {
            const QString candidateCidr = candidates.first().cidr();
            if (NetworkPolicy::isValidCidr(candidateCidr)) {
                cidrs.append(candidateCidr);
            }
        }
        shouldPersist = !cidrs.isEmpty();
    }

    m_networkPolicy->setMode(mode);
    m_networkPolicy->setSelectedInterfaces(toSet(selectedIds));
    m_networkPolicy->setAllowedCidrs(cidrs);

    const QString normalizedMode = NetworkPolicy::modeToString(mode);
    if (m_settings->networkMode() != normalizedMode) {
        if (m_settings->onboardingCompleted()) {
            m_settings->setNetworkMode(normalizedMode);
        }
    }
    if (shouldPersist && m_settings->onboardingCompleted()) {
        m_settings->setSelectedInterfaces(selectedIds.join(QStringLiteral(",")));
        m_settings->setAllowedCidrs(cidrs.join(QStringLiteral(",")));
    }
}

TransferRepository* Application::transferRepository() const
{
    return m_transferRepo;
}

ManualPeerRepository* Application::manualPeerRepository() const
{
    return m_manualPeerRepo;
}

} // namespace FengSui
