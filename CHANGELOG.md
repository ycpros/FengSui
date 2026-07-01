# Changelog

All notable changes to FengSui will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed
- Version management: CMake generates `Version.h` from `PROJECT_VERSION`, eliminating hardcoded version strings
- Settings page now displays app version; window title includes version number
- `--version` / `-v` CLI flag added
- Database schema version centralized in `Version.h.in`

## [0.5.0] — 2026-07-02

### Added
- **QML/Qt Quick UI** — Full migration from Qt Widgets to QML/Qt Quick with C++ ViewModel bridge layer
- **Contacts page** — Real-time online device list with OS badge, status pill, and manual IP connection
- **Chat page** — One-to-one text messaging with ChatBubble component, message status (sending/sent/failed), and SQLite persistence
- **Transfer Center** — Unified panel for all file transfers with filtering (all/in-progress/completed/failed) and progress bars
- **Share page** — Publish local folders, browse remote shares, download files, and access authorization dialog
- **Settings page** — Three-tab layout (General / Network / Diagnostics) with network mode selector, interface authorization, CIDR configuration
- **Onboarding wizard** — Four-step guided setup (display name, discovery toggle, network mode, download directory)
- **NetworkPolicy** — Network mode model (`secure_lan` / `multi_lan` / `compat_test`), CIDR filtering, interface enumeration
- **Manual peer persistence** — `manual_peers` table and `ManualPeerRepository` for saving manually added devices
- **TcpProbe** — TCP connectivity testing utility
- **Dark/light theme** — Theme.qml singleton with system-level and manual toggle
- **Software rendering fallback** — Automatic OpenGL detection and fallback for headless/remote desktop environments
- **`--screenshot` flag** — Developer tool for headless UI validation
- **9 test suites** — protocol, storage, signal_service, courier_service, share_service, beacon_policy, network_policy, qml_reflection, viewmodels

### Changed
- Widgets UI removed entirely; `QtWidgets` linked only for Windows system tray backend
- Architecture expanded to 5 layers: QML UI → C++ ViewModels → Core Services → Network / Storage
- Models layer uses `Q_GADGET` + `Q_PROPERTY` for QML reflection; enums use `Q_NAMESPACE` + `Q_ENUM_NS`
- Build system: `qt_add_qml_module` / `qt_add_executable`, `AUTOUIC` disabled

### Fixed
- Chat conversation refresh recursion bug

## [0.1.0] — Unreleased prototype

### Added
- Initial CMake + Qt 6 project skeleton
- SQLite database with full schema (peers, conversations, messages, transfer_tasks, shared_folders, access_grants, download_logs, settings)
- UDP peer discovery (multicast + heartbeat + goodbye)
- TCP messaging channel (`[4-byte BE length][JSON]` wire format)
- File transfer engine with chunked TCP (8 MB chunks, SHA-256 verification)
- Shared folder protocol (list / items / download over TCP JSON)
- BeaconService, SignalService, CourierService, ShareService
- PlatformUtils and InterfaceEnumerator
- Basic system tray icon
