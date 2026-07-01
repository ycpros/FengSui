# FengSui

English | [简体中文](README.zh-CN.md)

FengSui is a Qt 6 and C++17 desktop client for lightweight LAN messaging and file collaboration.
It is built for trusted local networks where teams need to discover nearby devices, send messages,
move files, and share folders without deploying a central server or depending on Internet access.

FengSui is not intended to replace enterprise chat platforms, cloud drives, or office automation
systems. Its goal is narrower: make the common "same network, need to talk and exchange files"
workflow simple, inspectable, and local-first.

## Project Status

FengSui is in early MVP development. The repository contains a functional QML/Qt Quick application
with full-featured pages for contacts, chat, transfer center, settings (including network diagnostics),
shared-folder management, and first-run onboarding. It includes local SQLite persistence, LAN discovery
and TCP messaging, file transfer workflows with progress tracking, and QtTest coverage for core services,
ViewModels, and protocol serialization.

The project is not yet a stable release. Protocols, UI details, database tables, and workflows may
change before v1.0.

## Features

- LAN peer discovery with UDP broadcast on the same subnet.
- Manual peer connection by IP address and port when broadcast discovery is unavailable, with TCP probe verification.
- One-to-one text conversations with local SQLite persistence.
- Individual file transfer over TCP, including accept, reject, cancel, progress, and history views.
- Transfer Center page for filtering and reviewing transfer tasks.
- Shared folder publishing, remote browsing, file download, and local access approval.
- First-run onboarding wizard for display name, discovery preference, and default download directory.
- Settings page with network policy configuration, interface enumeration, manual peer management, and built-in network diagnostics.
- Dark and light theme support with system-level switching.
- Software rendering fallback for environments without usable OpenGL (remote desktop, headless, older drivers).
- Cross-platform QML/Qt Quick UI targeting Windows, Linux, and macOS.
- Automated tests built with QtTest and CTest, covering core services, ViewModels, network protocol, and storage.

## Current Limitations

- FengSui currently assumes a trusted LAN. Transport is plaintext and does not provide end-to-end
  encryption, strong identity, or audit-grade access control.
- Automatic discovery is limited to the local subnet. Cross-VLAN and Internet traversal are out of
  scope for the current version.
- Folder drag-and-drop and recursive folder transfer are not complete yet.
- File dropbox workflows and enhanced system tray behavior are planned but not complete.
- There are no mobile clients, cloud relay services, organization management features, audio/video
  calls, or collaborative document editing.

## Tech Stack

- Language: C++17
- UI: Qt 6 QML / Qt Quick
- Networking: Qt Network with UDP and TCP
- Storage: SQLite through Qt SQL
- Build: CMake 3.20+
- Tests: QtTest and CTest
- Target platforms: Windows, Linux, and macOS

The v1 line intentionally avoids Electron, Flutter, web front-end frameworks, Qt WebEngine,
third-party network libraries, and background service dependencies. On Windows, the system tray
uses QtWidgets as the backend for Qt.labs.platform/QSystemTrayIcon while the application UI
remains QML/Qt Quick.

The application includes a dark/light theme system (switchable at runtime via `--theme dark|light`),
a software rendering fallback for headless or remote desktop environments, and a `--screenshot`
developer flag for capturing window snapshots in CI or headless validation.

## Build From Source

### Requirements

- Qt 6.8 or newer
- CMake 3.20 or newer
- A C++17-capable compiler
- Ninja is optional but recommended for local debug builds

### Configure

```bash
cmake -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

### Build

```bash
cmake --build cmake-build-debug
```

### Run

Windows:

```powershell
.\cmake-build-debug\fengsui.exe
```

Linux or macOS:

```bash
./cmake-build-debug/fengsui
```

### Test

```bash
ctest --test-dir cmake-build-debug --output-on-failure
```

## Repository Layout

```text
src/
├── main.cpp    Application entry point
├── app/        Application class, settings facade, and logging
├── core/       Business services (Beacon, Signal, Courier, Share, TcpProbe)
├── models/     Shared data structures and enums (Q_GADGET / Q_NAMESPACE)
├── network/    UDP discovery, TCP connection/server, and protocol serialization
├── platform/   Platform detection, host info, and network interface enumeration
├── storage/    SQLite database and repository classes
├── tests/      QtTest targets (services, ViewModels, protocol, storage, QML reflection)
└── ui/
    ├── qml/           Application shell, theme singleton, and JS utilities
    ├── components/    Reusable QML components (buttons, cards, bubbles, inputs, etc.)
    ├── pages/         QML pages (Contacts, Chat, TransferCenter, Settings, Share, Onboarding)
    ├── dialogs/       QML dialogs (AddPeer, IncomingTransfer, ShareAccess)
    ├── viewmodels/    C++ ViewModels and list models bridging QML ↔ core services
    └── assets/        Static resources (tray icon, etc.)
```

Layering conventions:

- `ui/qml/` renders the UI and binds to ViewModel properties; it never calls core services directly.
- `ui/viewmodels/` bridges QML and core: exposes data via `Q_PROPERTY` and `QAbstractListModel`, forwards user actions to core services, and relays service signals back to QML.
- `core/` owns business behavior and may call `network/` and `storage/`.
- `network/` owns sockets, connections, and protocol payloads.
- `storage/` is the only database access layer and returns data through `models/`.
- `models/` stays lightweight (pure structs and enums) and is shared across all layers.

## Contributing

Contributions are welcome while the project is still taking shape. Please keep changes focused and
consistent with the current architecture:

- Use QML/Qt Quick for UI work. QtWidgets is linked only for the Windows system tray backend.
- Use C++ ViewModels (`QAbstractListModel` / `QObject` with `QML_ELEMENT`) to bridge QML and core services.
- Expose shared enums to QML via `QML_FOREIGN_NAMESPACE` in `ui/viewmodels/QmlEnums.h`.
- Use Qt Network for networking.
- Use `QJsonDocument` and related Qt JSON types for protocol serialization.
- Keep UI and ViewModel code out of `core/`, `network/`, and `storage/`.
- Add or update QtTest coverage for behavior changes (core services, ViewModels, protocol, or storage).
- Prefer clear, local changes over broad refactors.

## Security Notes

FengSui is designed for trusted local networks. Do not use the current version for confidential,
regulated, hostile, or cross-organization environments without adding stronger authentication,
encrypted transport, access auditing, and administrative controls.

## License

FengSui is released under the MIT License. See [LICENSE](LICENSE) for details.
