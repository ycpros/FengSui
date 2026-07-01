# FengSui

English | [简体中文](README.zh-CN.md)

FengSui is a Qt 6 and C++17 desktop client for lightweight LAN messaging and file collaboration.
It is built for trusted local networks where teams need to discover nearby devices, send messages,
move files, and share folders without deploying a central server or depending on Internet access.

FengSui is not intended to replace enterprise chat platforms, cloud drives, or office automation
systems. Its goal is narrower: make the common "same network, need to talk and exchange files"
workflow simple, inspectable, and local-first.

## Project Status

FengSui is in early MVP development. The repository contains a working Qt Widgets application
shell, local persistence, LAN discovery and TCP messaging foundations, file transfer workflows,
shared-folder browsing, and QtTest coverage for core behaviors.

The project is not yet a stable release. Protocols, UI details, database tables, and workflows may
change before v1.0.

## Features

- LAN peer discovery with UDP broadcast on the same subnet.
- Manual peer connection by IP address and port when broadcast discovery is unavailable.
- One-to-one text conversations with local SQLite persistence.
- Individual file transfer over TCP, including accept, reject, cancel, progress, and history views.
- Transfer Center page for filtering and reviewing transfer tasks.
- Shared folder publishing, remote browsing, file download, and local access approval.
- First-run onboarding for display name, discovery preference, and default download directory.
- Settings and diagnostics views for network policy, interfaces, manual peers, and basic checks.
- Cross-platform Qt Widgets UI targeting Windows, Linux, and macOS.
- Automated tests built with QtTest and CTest.

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
- UI: Qt 6 Widgets
- Networking: Qt Network with UDP and TCP
- Storage: SQLite through Qt SQL
- Build: CMake 3.20+
- Tests: QtTest and CTest
- Target platforms: Windows, Linux, and macOS

The v1 line intentionally avoids QML, Qt Quick, Electron, Flutter, web front-end frameworks, third-party
network libraries, and background service dependencies.

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
├── app/        Application startup, settings, and logging
├── core/       Business services for discovery, messaging, sharing, and transfer
├── models/     Shared data structures
├── network/    UDP/TCP networking and protocol serialization
├── platform/   Platform-specific helpers
├── storage/    SQLite database and repository classes
├── tests/      QtTest test targets
└── ui/         Qt Widgets windows, pages, and dialogs
```

Layering conventions:

- `ui/` handles presentation and user interaction, and calls services from `core/`.
- `core/` owns business behavior and may call `network/` and `storage/`.
- `network/` owns sockets, connections, and protocol payloads.
- `storage/` is the only database access layer and returns data through `models/`.
- `models/` stays lightweight and shared across layers.

## Contributing

Contributions are welcome while the project is still taking shape. Please keep changes focused and
consistent with the current architecture:

- Use Qt Widgets for UI work.
- Use Qt Network for networking.
- Use `QJsonDocument` and related Qt JSON types for protocol serialization.
- Keep QWidget code out of `core/`, `network/`, and `storage/`.
- Add or update QtTest coverage for behavior changes.
- Prefer clear, local changes over broad refactors.

## Security Notes

FengSui is designed for trusted local networks. Do not use the current version for confidential,
regulated, hostile, or cross-organization environments without adding stronger authentication,
encrypted transport, access auditing, and administrative controls.

## License

FengSui is released under the MIT License. See [LICENSE](LICENSE) for details.
