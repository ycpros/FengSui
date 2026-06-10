cmake_minimum_required(VERSION 3.20)
project(FengSui VERSION 0.1.0 LANGUAGES CXX)

# C++17 标准，按 10_coding_rules.md 要求
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 源文件统一按 UTF-8 读取，允许直接书写中文 UI 文案
if(MSVC)
    add_compile_options(/utf-8)
else()
    add_compile_options(-finput-charset=UTF-8)
endif()

# Qt 自动处理 MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找 Qt6 模块（应用目标仅链接 Widgets / Network / Sql；Test 仅供自动化测试目标使用）
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Sql Test)

# 源文件列表
set(SOURCES
    src/main.cpp
    # app 层
    src/app/Application.cpp
    src/app/Logger.cpp
    src/app/AppSettings.cpp
    # storage 层
    src/storage/ConversationRepository.cpp
    src/storage/Database.cpp
    src/storage/ManualPeerRepository.cpp
    src/storage/MessageRepository.cpp
    src/storage/SettingsRepository.cpp
    src/storage/TransferRepository.cpp
    # ui 层
    src/ui/MainWindow.cpp
    src/ui/onboarding/OnboardingWizard.cpp
    src/ui/chat/ChatPage.cpp
    src/ui/contacts/ContactsPage.cpp
    src/ui/contacts/AddPeerDialog.cpp
    src/ui/transfer_center/TransferCenterPage.cpp
    src/ui/share/SharePage.cpp
    src/ui/settings/SettingsPage.cpp
    src/ui/diagnostics/DiagnosticsPage.cpp
    src/ui/dropbox/DropboxPage.cpp
    src/ui/tray/TrayManager.cpp
    # core 层
    src/core/BeaconService.cpp
    src/core/CourierService.cpp
    src/core/SignalService.cpp
    src/core/TcpProbe.cpp
    # network 层
    src/network/UdpDiscovery.cpp
    src/network/Protocol.cpp
    src/network/TcpConnection.cpp
    src/network/TcpServer.cpp
    # platform 层
    src/platform/InterfaceEnumerator.h
    src/platform/PlatformUtils.cpp
    src/platform/InterfaceEnumerator.cpp
)

set(HEADERS
    # app 层
    src/app/Application.h
    src/app/Logger.h
    src/app/AppSettings.h
    # models（纯头文件）
    src/models/NetworkPolicy.h
    src/models/PeerInfo.h
    src/models/Message.h
    src/models/Conversation.h
    src/models/TransferTask.h
    src/models/SharedFolder.h
    src/models/AccessGrant.h
    src/models/DownloadLog.h
    # storage 层
    src/storage/ConversationRepository.h
    src/storage/Database.h
    src/storage/ManualPeerRepository.h
    src/storage/MessageRepository.h
    src/storage/SettingsRepository.h
    src/storage/TransferRepository.h
    # ui 层
    src/ui/MainWindow.h
    src/ui/onboarding/OnboardingWizard.h
    src/ui/chat/ChatPage.h
    src/ui/contacts/ContactsPage.h
    src/ui/contacts/AddPeerDialog.h
    src/ui/transfer_center/TransferCenterPage.h
    src/ui/share/SharePage.h
    src/ui/settings/SettingsPage.h
    src/ui/diagnostics/DiagnosticsPage.h
    src/ui/dropbox/DropboxPage.h
    src/ui/tray/TrayManager.h
    # core 层
    src/core/BeaconService.h
    src/core/CourierService.h
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

# 可执行文件
add_executable(fengsui ${SOURCES} ${HEADERS})

# 链接 Qt 库
target_link_libraries(fengsui PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
)

# 项目内部 include 路径
file(REAL_PATH "${CMAKE_SOURCE_DIR}/src" FENGSUI_SRC_DIR)
target_include_directories(fengsui PRIVATE "${FENGSUI_SRC_DIR}")

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

function(fengsui_add_qttest target_name)
    add_executable(${target_name} ${ARGN})
    target_include_directories(${target_name} PRIVATE "${FENGSUI_SRC_DIR}")
    target_link_libraries(${target_name} PRIVATE
        Qt6::Test
        Qt6::Widgets
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
    src/storage/ConversationRepository.cpp
    src/storage/Database.cpp
    src/storage/ManualPeerRepository.cpp
    src/storage/MessageRepository.cpp
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

fengsui_add_qttest(tst_add_peer_dialog
    src/tests/tst_add_peer_dialog.cpp
    src/core/TcpProbe.cpp
    src/ui/contacts/AddPeerDialog.cpp
)

fengsui_add_qttest(tst_beacon_policy
    src/tests/tst_beacon_policy.cpp
    src/app/AppSettings.cpp
    src/core/BeaconService.cpp
    src/network/UdpDiscovery.cpp
    src/platform/InterfaceEnumerator.cpp
    src/platform/PlatformUtils.cpp
    src/storage/Database.cpp
    src/storage/SettingsRepository.cpp
)
