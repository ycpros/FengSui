# 🔥 烽燧 FengSui

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Qt](https://img.shields.io/badge/Qt-6.8%2B-green.svg)](https://www.qt.io/)
[![C++](https://img.shields.io/badge/C%2B%2B-17-00599C.svg)](https://en.cppreference.com/)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)](#)
[![Build](https://img.shields.io/badge/build-CMake-064F8C.svg)](https://cmake.org/)

English | [简体中文](README.zh-CN.md)

---

**FengSui** is a lightweight, serverless LAN messaging and file-sharing desktop application built with Qt 6 and C++17. It helps teams on the same local network discover each other, chat, transfer files, and share folders — no cloud, no Internet, no server deployment required.

> FengSui is not an enterprise chat platform, cloud drive, or office suite. Its goal is narrower and simpler: make the everyday "same network, need to talk and exchange files" workflow fast, local-first, and inspectable.

---

## 📋 Table of Contents

- [✨ Features](#-features)
- [🖼️ Screenshots](#-screenshots)
- [🚀 Quick Start](#-quick-start)
- [🏗️ Architecture](#-architecture)
- [📦 Build From Source](#-build-from-source)
- [📁 Repository Layout](#-repository-layout)
- [🧪 Tests](#-tests)
- [⚠️ Current Limitations](#-current-limitations)
- [🤝 Contributing](#-contributing)
- [🔒 Security Notes](#-security-notes)
- [📄 License](#-license)

---

## ✨ Features

### 🔍 Discovery
- **UDP broadcast auto-discovery** — peers on the same subnet appear within seconds
- **Manual IP connection** — add peers by `IP:port` when broadcast is unavailable, with TCP probe verification

### 💬 Messaging
- **One-to-one text chat** — real-time conversations with local SQLite persistence
- **Message history** — all conversations survive restarts; scroll back to any point

### 📎 File Transfer
- **Single-file transfer over TCP** — drag a file, send it; supports accept / reject / cancel
- **Real-time progress** — percentage, speed, and estimated time displayed during transfer
- **Transfer Center** — unified panel to filter, sort, and review all past and active transfers

### 📂 Shared Folders
- **Publish local folders** — share any directory with one click
- **Remote browsing** — browse shared directories from other peers, navigate subdirectories
- **Download on demand** — pick individual files from a shared folder
- **Access approval** — grant or deny access when someone requests your shared files

### ⚙️ System
- **Onboarding wizard** — guided 3-step setup: display name → discovery preference → download directory
- **Settings & diagnostics** — network policy, interface list, manual peer management, built-in connectivity checks
- **Dark / light theme** — switch at runtime or default to the system setting (`--theme dark|light`)
- **Software rendering fallback** — works on remote desktop, headless, and older drivers without usable OpenGL
- **Cross-platform** — Windows, Linux (Ubuntu/Debian/openKylin), and macOS

---

## 🖼️ Screenshots

| Messages                                           | Contacts                                               |
|----------------------------------------------------|--------------------------------------------------------|
| ![Messages page](assets/screenshots/chat-dark.png) | ![Contacts page](assets/screenshots/contacts-dark.png) |

| Transfer Center                                               | Shared Files                                            |
|---------------------------------------------------------------|---------------------------------------------------------|
| ![Transfer Center page](assets/screenshots/transfer-dark.png) | ![Shared Files page](assets/screenshots/share-dark.png) |

Generate updated snapshots for development review:

```bash
./fengsui --screenshot assets/screenshots/chat-dark.png --screenshot-page chat --theme dark
```

---

## 🚀 Quick Start

### Prerequisites

| Requirement | Version |
|---|---|
| Qt | 6.8 or newer |
| CMake | 3.20 or newer |
| Compiler | C++17-capable (MSVC 2022, GCC 11+, Clang 14+) |
| Ninja | Optional, recommended for local builds |

### One-liner Build

```bash
cmake -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  && cmake --build cmake-build-debug
```

### Run

```bash
# Windows
.\cmake-build-debug\fengsui.exe

# Linux / macOS
./cmake-build-debug/fengsui
```

Start two instances on the same LAN (or same machine with different ports) to test discovery and messaging.

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────┐
│  QML / Qt Quick UI                      │
│  pages · components · dialogs           │
│  (Main.qml → AppController singleton)   │
├─────────────────────────────────────────┤
│  C++ ViewModel Bridge                   │
│  QAbstractListModel / QObject            │
│  QML_ELEMENT · QML_FOREIGN_NAMESPACE    │
├─────────────────────────────────────────┤
│  Application Services                   │
│  Beacon · Signal · Courier · Share      │
│  TcpProbe                               │
├─────────────────────────────────────────┤
│  Network                 │  Storage     │
│  UDP · TCP · Protocol    │  SQLite      │
└─────────────────────────────────────────┘
```

**Data flow:** `QML View` → `ViewModel` → `Core Service` → `Network / Storage`

Each layer has clear boundaries:
- **ui/qml** — declarative UI, binds to ViewModel properties only
- **ui/viewmodels** — C++ bridge, translates between QML and core services
- **core** — business logic, no UI dependencies
- **network** — raw socket I/O and protocol serialization
- **storage** — sole SQLite access layer, returns plain structs

---

## 📦 Build From Source

### Requirements

- **Qt 6.8+** with modules: Core, Gui, Widgets, Qml, Quick, QuickControls2, Network, Sql
- **CMake 3.20+**
- **C++17 compiler** (MSVC 2022, GCC 11+, or Clang 14+)
- **Ninja** (optional, recommended)

### Configure

```bash
cmake -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

### Build

```bash
cmake --build cmake-build-debug
```

### Run

**Windows:**
```powershell
.\cmake-build-debug\fengsui.exe
```

**Linux / macOS:**
```bash
./cmake-build-debug/fengsui
```

### Developer Flags

| Flag | Description |
|---|---|
| `--theme dark\|light` | Override the system theme |
| `--screenshot <path>` | Capture a window snapshot and exit (CI / headless) |
| `--screenshot-page <page>` | Select the page for screenshots: `chat`, `contacts`, `transfer`, `share`, or `settings` |

---

## 📁 Repository Layout

```
src/
├── main.cpp              Application entry point
├── app/                  Application class, settings facade, logging
├── core/                 Business services
│   ├── BeaconService     UDP peer discovery
│   ├── SignalService     TCP messaging channel
│   ├── CourierService    File transfer engine
│   ├── ShareService      Shared folder management
│   └── TcpProbe          Connectivity verification
├── models/               Shared structs & enums (Q_GADGET / Q_NAMESPACE)
├── network/              Network layer
│   ├── UdpDiscovery      UDP broadcast / multicast
│   ├── TcpConnection     Per-peer TCP connection
│   ├── TcpServer         Inbound TCP listener
│   └── Protocol          Message serialization (JSON)
├── storage/              SQLite persistence layer
│   ├── Database          Connection & schema management
│   ├── SettingsRepository
│   ├── ManualPeerRepository
│   ├── ConversationRepository
│   ├── MessageRepository
│   ├── TransferRepository
│   ├── ShareRepository
│   ├── AccessGrantRepository
│   └── DownloadLogRepository
├── platform/             OS-specific utilities
│   ├── PlatformUtils     Hostname, paths, platform detection
│   └── InterfaceEnumerator  Network interface listing
├── tests/                QtTest targets
│   ├── tst_protocol, tst_storage
│   ├── tst_signal_service, tst_courier_service
│   ├── tst_share_service, tst_beacon_policy
│   ├── tst_network_policy
│   ├── tst_qml_reflection
│   └── tst_viewmodels
└── ui/                   QML frontend + C++ ViewModels
    ├── qml/              Shell (Main.qml), theme, JS utils
    ├── components/       Reusable controls (13 components)
    ├── pages/            6 pages (Contacts, Chat, TransferCenter,
    │                       Settings, Share, Onboarding)
    ├── dialogs/          3 dialogs (AddPeer, IncomingTransfer,
    │                       ShareAccess)
    ├── viewmodels/       11 ViewModel classes bridging QML ↔ core
    └── assets/           Tray icon & static resources
```

---

## 🧪 Tests

```bash
ctest --test-dir cmake-build-debug --output-on-failure
```

| Suite | Coverage |
|---|---|
| `tst_protocol` | Message serialization / deserialization |
| `tst_storage` | SQLite CRUD across all repositories |
| `tst_signal_service` | TCP messaging channel |
| `tst_courier_service` | File transfer engine |
| `tst_share_service` | Shared folder logic |
| `tst_beacon_policy` | Discovery policy & UDP behavior |
| `tst_network_policy` | Network interface & policy |
| `tst_qml_reflection` | Q_GADGET / Q_ENUM_NS reflection + QML enum access |
| `tst_viewmodels` | ViewModel list-model incremental behavior |

---

## ⚠️ Current Limitations

| Area | Status |
|---|---|
| Encryption | Plaintext only — trusted LAN assumed for v1.0 |
| Cross-subnet | Discovery limited to local subnet; no VLAN / Internet traversal |
| Folder transfer | Recursive folder drag-and-drop not yet complete |
| File dropbox | P1 — planned, not yet implemented |
| System tray | Basic tray icon; enhanced behavior (minimize-to-tray, status indicator) planned |
| Browser guest | P1 — HTTP guest access for non-client users planned |
| Mobile / cloud | Out of scope for the desktop client |

---

## 🤝 Contributing

Contributions are welcome! Please keep changes focused and aligned with the architecture:

- **UI** → QML / Qt Quick (`QtWidgets` is linked only for the Windows tray backend)
- **Bridge** → C++ ViewModels (`QAbstractListModel` / `QObject` + `QML_ELEMENT`)
- **Enums** → Expose shared enums via `QML_FOREIGN_NAMESPACE` in `ui/viewmodels/QmlEnums.h`
- **Network** → Qt Network only (`QTcpSocket` / `QUdpSocket`)
- **Serialization** → `QJsonDocument` (no Protobuf / MessagePack)
- **Separation** → Keep UI and ViewModel code out of `core/`, `network/`, and `storage/`
- **Tests** → Add or update QtTest coverage for behavior changes
- **Scope** → Prefer focused, local changes over sweeping refactors

See [CLAUDE.md](CLAUDE.md) and the [docs/](docs/) directory for detailed conventions.

---

## 🔒 Security Notes

FengSui is designed for **trusted local networks**. The current version transmits data in plaintext and does not provide end-to-end encryption, strong authentication, or audit-grade access control. Do not use it for confidential, regulated, or cross-organization environments without adding those layers.

---

## 📄 License

FengSui is released under the [MIT License](LICENSE).

---

<p align="center">
  <sub>Built with Qt 6 · C++17 · zero cloud dependencies</sub>
</p>
