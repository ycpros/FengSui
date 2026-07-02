cmake_minimum_required(VERSION 3.20)
project(FengSui VERSION 0.5.0 LANGUAGES CXX)

# C++17 标准，按 10_coding_rules.md 要求
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 源文件统一按 UTF-8 读取，允许直接书写中文 UI 文案
if(MSVC)
    add_compile_options(/utf-8)
else()
    add_compile_options(-finput-charset=UTF-8)
endif()

# Qt 自动处理 MOC/RCC（迁移到 QML 后不再使用 .ui，故关闭 AUTOUIC）
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

# 查找 Qt6 模块。UI 迁移到 QML：应用目标链接 Quick / QuickControls2 / Qml。
# Widgets 仅作为 Windows 托盘（Qt.labs.platform -> QSystemTrayIcon）的后端依赖，
# 旧 Qt Widgets 页面已删除；Network / Sql 供下层使用；Test 仅供自动化测试目标使用。
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml Quick QuickControls2 Network Sql Test)

# 启用 Qt 标准工程设置（qt_add_qml_module 等命令需要，要求 Qt 6.8+）
qt_standard_project_setup(REQUIRES 6.8)

# 应用后端源文件（非 QML/ViewModel 部分）。
# 旧 Qt Widgets UI 已删除；UI 由 QML + ViewModel 承载。
# models 层元对象编译单元 + 头文件。models 为纯头（Q_GADGET/Q_NAMESPACE），
# Models.cpp 通过 include 触发 AUTOMOC 生成 staticMetaObject。凡引用这些类型元对象
# 的目标（主程序、各测试）都需链接此列表，否则链接期报 staticMetaObject 未解析。
set(MODEL_META_SOURCES
    src/models/Models.cpp
    src/models/ModelEnums.h
    src/models/PeerInfo.h
    src/models/Message.h
    src/models/Conversation.h
    src/models/TransferTask.h
    src/models/SharedFolder.h
    src/models/AccessGrant.h
    src/models/DownloadLog.h
)

set(SOURCES
    src/main.cpp
    # models 层元对象（见 MODEL_META_SOURCES 说明）
    ${MODEL_META_SOURCES}
    # app 层
    src/app/Application.cpp
    src/app/Logger.cpp
    src/app/AppSettings.cpp
    # storage 层
    src/storage/AccessGrantRepository.cpp
    src/storage/ConversationRepository.cpp
    src/storage/Database.cpp
    src/storage/DownloadLogRepository.cpp
    src/storage/ManualPeerRepository.cpp
    src/storage/MessageRepository.cpp
    src/storage/SettingsRepository.cpp
    src/storage/ShareRepository.cpp
    src/storage/TransferRepository.cpp
    # core 层
    src/core/BeaconService.cpp
    src/core/CourierService.cpp
    src/core/ShareService.cpp
    src/core/SignalService.cpp
    src/core/TcpProbe.cpp
    # network 层
    src/network/UdpDiscovery.cpp
    src/network/Protocol.cpp
    src/network/TcpConnection.cpp
    src/network/TcpServer.cpp
    # platform 层
    src/platform/PlatformUtils.cpp
    src/platform/InterfaceEnumerator.cpp
)

# ui/viewmodels 层源文件（ViewModel + QAbstractListModel）。
# 阶段 2-3 逐步填充，此处集中管理便于 qt_add_qml_module 的 SOURCES 引用。
set(VIEWMODEL_SOURCES
    src/ui/viewmodels/QmlEnums.h        # 把 models 枚举以 QML_FOREIGN_NAMESPACE 暴露为 Enums
    src/ui/viewmodels/ThemeController.h
    src/ui/viewmodels/ThemeController.cpp
    src/ui/viewmodels/AppController.h
    src/ui/viewmodels/AppController.cpp
    src/ui/viewmodels/PeerListModel.h
    src/ui/viewmodels/PeerListModel.cpp
    src/ui/viewmodels/ContactsViewModel.h
    src/ui/viewmodels/ContactsViewModel.cpp
    src/ui/viewmodels/TransferListModel.h
    src/ui/viewmodels/TransferListModel.cpp
    src/ui/viewmodels/TransferViewModel.h
    src/ui/viewmodels/TransferViewModel.cpp
    src/ui/viewmodels/SettingsViewModel.h
    src/ui/viewmodels/SettingsViewModel.cpp
    src/ui/viewmodels/ShareViewModel.h
    src/ui/viewmodels/ShareViewModel.cpp
    src/ui/viewmodels/ConversationListModel.h
    src/ui/viewmodels/ConversationListModel.cpp
    src/ui/viewmodels/MessageListModel.h
    src/ui/viewmodels/MessageListModel.cpp
    src/ui/viewmodels/ChatViewModel.h
    src/ui/viewmodels/ChatViewModel.cpp
    src/ui/viewmodels/OnboardingViewModel.h
    src/ui/viewmodels/OnboardingViewModel.cpp
)

# QML 文件列表。阶段 2-4 逐步填充。
set(QML_FILES
    src/ui/qml/Main.qml
    src/ui/qml/Theme.qml
    # 通用组件
    src/ui/components/AppIcon.qml
    src/ui/components/NavItem.qml
    src/ui/components/PrimaryButton.qml
    src/ui/components/StatusPill.qml
    src/ui/components/EmptyState.qml
    src/ui/components/PageHeader.qml
    src/ui/components/OsBadge.qml
    src/ui/components/DeviceCard.qml
    src/ui/components/AppComboBox.qml
    src/ui/components/AppTextField.qml
    src/ui/components/ThinProgressBar.qml
    src/ui/components/AppSwitch.qml
    src/ui/components/SectionCard.qml
    src/ui/components/SettingRow.qml
    src/ui/components/ChatBubble.qml
    # 页面
    src/ui/pages/ContactsPage.qml
    src/ui/pages/TransferCenterPage.qml
    src/ui/pages/SettingsPage.qml
    src/ui/pages/SharePage.qml
    src/ui/pages/ChatPage.qml
    src/ui/pages/OnboardingWizard.qml
    # 对话框
    src/ui/dialogs/AddPeerDialog.qml
    src/ui/dialogs/ShareAccessDialog.qml
    src/ui/dialogs/IncomingTransferDialog.qml
    # JS 工具
    src/ui/qml/Format.js
)

# Theme.qml 是 pragma Singleton，需向 qt_add_qml_module 声明其为单例类型
set_source_files_properties(src/ui/qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

# 非 QML 资源（托盘图标等），以固定别名打进模块资源，QML 用 qrc 路径引用
set(QML_RESOURCES
    src/ui/assets/tray.svg
)
set_source_files_properties(src/ui/assets/tray.svg PROPERTIES QT_RESOURCE_ALIAS assets/tray.svg)

set(HEADERS
    # app 层
    src/app/Application.h
    src/app/Logger.h
    src/app/AppSettings.h
    # models 纯头（含 Q_GADGET 的头改由 MODEL_META_SOURCES 统一管理，此处仅列无元对象的）
    src/models/NetworkPolicy.h
    # storage 层
    src/storage/AccessGrantRepository.h
    src/storage/ConversationRepository.h
    src/storage/Database.h
    src/storage/DownloadLogRepository.h
    src/storage/ManualPeerRepository.h
    src/storage/MessageRepository.h
    src/storage/SettingsRepository.h
    src/storage/ShareRepository.h
    src/storage/TransferRepository.h
    # core 层
    src/core/BeaconService.h
    src/core/CourierService.h
    src/core/ShareService.h
    src/core/SignalService.h
    src/core/TcpProbe.h
    # network 层
    src/network/UdpDiscovery.h
    src/network/Protocol.h
    src/network/TcpConnection.h
    src/network/TcpServer.h
    # platform 层
    src/platform/InterfaceEnumerator.h
    src/platform/PlatformUtils.h
)

# 根据 CMake PROJECT_VERSION 生成版本头文件。
# 应用程序通过 #include "Version.h" 使用 FENGSUI_VERSION_* 宏，
# 消除 Application.cpp 中硬编码版本字符串的重复。
configure_file(
    "${CMAKE_SOURCE_DIR}/src/app/Version.h.in"
    "${CMAKE_BINARY_DIR}/generated/Version.h"
)

# 可执行文件（QML 应用需用 qt_add_executable 以正确处理资源与元数据）
qt_add_executable(fengsui ${SOURCES} ${HEADERS} ${VIEWMODEL_SOURCES})

# 注册 QML 模块。QML 文件与带 QML_ELEMENT 的 ViewModel 归入 URI "FengSui.Ui"，
# QML 中通过 import FengSui.Ui 访问，main.cpp 用 loadFromModule 加载 Main.qml。
qt_add_qml_module(fengsui
    URI FengSui.Ui
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    QML_FILES ${QML_FILES}
    SOURCES ${VIEWMODEL_SOURCES}
    RESOURCES ${QML_RESOURCES}
)

# 链接 Qt 库
target_link_libraries(fengsui PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::Sql
)

# 项目内部 include 路径。除 src 根外，额外暴露 viewmodels 目录，
# 使 qt_add_qml_module 生成的类型注册文件能以 <QmlEnums.h> 定位到外部命名空间头。
file(REAL_PATH "${CMAKE_SOURCE_DIR}/src" FENGSUI_SRC_DIR)
target_include_directories(fengsui PRIVATE
    "${FENGSUI_SRC_DIR}"
    "${FENGSUI_SRC_DIR}/ui/viewmodels"
    "${CMAKE_BINARY_DIR}/generated"     # Version.h（configure_file 生成）
)

# Windows 下隐藏控制台窗口（桌面应用）
if(WIN32)
    set_target_properties(fengsui PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# 平台检测宏，方便后续 platform/ 层条件编译
if(WIN32)
    target_compile_definitions(fengsui PRIVATE FENGSUI_PLATFORM_WINDOWS)
elseif(APPLE)
    target_compile_definitions(fengsui PRIVATE FENGSUI_PLATFORM_MACOS)
elseif(UNIX)
    target_compile_definitions(fengsui PRIVATE FENGSUI_PLATFORM_LINUX)
endif()

# ---- 自动化测试 ----

enable_testing()

# 测试目标辅助函数。UI 迁移到 QML 后不再链接 Widgets；
# 保留的 core/network/storage 测试仅依赖 Test/Network/Sql。
# 统一追加 MODEL_META_SOURCES，使各测试能解析 models 层 struct 的 staticMetaObject。
function(fengsui_add_qttest target_name)
    add_executable(${target_name} ${ARGN} ${MODEL_META_SOURCES})
    target_include_directories(${target_name} PRIVATE
        "${FENGSUI_SRC_DIR}"
        "${CMAKE_BINARY_DIR}/generated"     # Version.h
    )
    target_link_libraries(${target_name} PRIVATE
        Qt6::Test
        Qt6::Network
        Qt6::Sql
    )
    add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()

fengsui_add_qttest(tst_network_policy
    src/tests/tst_network_policy.cpp
    src/platform/InterfaceEnumerator.cpp
)

fengsui_add_qttest(tst_protocol
    src/tests/tst_protocol.cpp
    src/network/Protocol.cpp
)

fengsui_add_qttest(tst_storage
    src/tests/tst_storage.cpp
    src/storage/AccessGrantRepository.cpp
    src/storage/ConversationRepository.cpp
    src/storage/Database.cpp
    src/storage/DownloadLogRepository.cpp
    src/storage/ManualPeerRepository.cpp
    src/storage/MessageRepository.cpp
    src/storage/ShareRepository.cpp
)

fengsui_add_qttest(tst_signal_service
    src/tests/tst_signal_service.cpp
    src/app/AppSettings.cpp
    src/core/CourierService.cpp
    src/core/SignalService.cpp
    src/network/Protocol.cpp
    src/network/TcpConnection.cpp
    src/network/TcpServer.cpp
    src/platform/InterfaceEnumerator.cpp
    src/storage/ConversationRepository.cpp
    src/storage/Database.cpp
    src/storage/MessageRepository.cpp
    src/storage/SettingsRepository.cpp
    src/storage/TransferRepository.cpp
)

fengsui_add_qttest(tst_courier_service
    src/tests/tst_courier_service.cpp
    src/app/AppSettings.cpp
    src/core/CourierService.cpp
    src/core/SignalService.cpp
    src/network/Protocol.cpp
    src/network/TcpConnection.cpp
    src/network/TcpServer.cpp
    src/platform/InterfaceEnumerator.cpp
    src/storage/ConversationRepository.cpp
    src/storage/Database.cpp
    src/storage/MessageRepository.cpp
    src/storage/SettingsRepository.cpp
    src/storage/TransferRepository.cpp
)

# 注：依赖旧 Qt Widgets 页面的 UI 测试（tst_transfer_center_page / tst_chat_page /
# tst_add_peer_dialog / tst_settings_page / tst_share_page / tst_diagnostics_page /
# tst_share_access_dialog / tst_main_window_navigation）已随 Widgets UI 一并移除。
# 后续将对 ViewModel 层补写 QtTest 单测。core/network/storage 测试保留如下。

fengsui_add_qttest(tst_share_service
    src/tests/tst_share_service.cpp
    src/app/AppSettings.cpp
    src/core/ShareService.cpp
    src/network/Protocol.cpp
    src/network/TcpConnection.cpp
    src/storage/AccessGrantRepository.cpp
    src/storage/Database.cpp
    src/storage/DownloadLogRepository.cpp
    src/storage/SettingsRepository.cpp
    src/storage/ShareRepository.cpp
)

# 阶段 1 数据桥接底座测试：Q_GADGET 属性反射 + Q_ENUM_NS 命名空间枚举反射 +
# QML 端到端枚举访问。MODEL_META_SOURCES 已由辅助函数统一追加。
# 该测试额外需要 Qt6::Qml（QQmlEngine/QQmlComponent + 命名空间注册）。
fengsui_add_qttest(tst_qml_reflection
    src/tests/tst_qml_reflection.cpp
)
target_link_libraries(tst_qml_reflection PRIVATE Qt6::Qml)

# 阶段 3-4 ViewModel 列表模型增量行为测试（纯模型，不依赖 Service）。
fengsui_add_qttest(tst_viewmodels
    src/tests/tst_viewmodels.cpp
    src/ui/viewmodels/PeerListModel.h
    src/ui/viewmodels/PeerListModel.cpp
    src/ui/viewmodels/TransferListModel.h
    src/ui/viewmodels/TransferListModel.cpp
    src/ui/viewmodels/MessageListModel.h
    src/ui/viewmodels/MessageListModel.cpp
    src/ui/viewmodels/ConversationListModel.h
    src/ui/viewmodels/ConversationListModel.cpp
)

fengsui_add_qttest(tst_beacon_policy
    src/tests/tst_beacon_policy.cpp
    src/app/AppSettings.cpp
    src/core/BeaconService.cpp
    src/core/ShareService.cpp
    src/network/Protocol.cpp
    src/network/TcpConnection.cpp
    src/network/UdpDiscovery.cpp
    src/platform/InterfaceEnumerator.cpp
    src/platform/PlatformUtils.cpp
    src/storage/AccessGrantRepository.cpp
    src/storage/Database.cpp
    src/storage/DownloadLogRepository.cpp
    src/storage/SettingsRepository.cpp
    src/storage/ShareRepository.cpp
)
