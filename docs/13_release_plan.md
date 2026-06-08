# 13 — 打包发布计划

## 版本号规则

`主版本.次版本.修订号`，遵循语义化版本：

| 阶段 | 版本 |
|---|---|
| 工程准备 | 0.1.0 |
| UI 壳子完成 | 0.2.0 |
| 发现 + 消息闭环 | 0.3.0 |
| 文件传输闭环 | 0.4.0 |
| 共享目录闭环 | 0.5.0 |
| P0 全部完成 | 0.9.0 |
| 测试 + 修复完成 | 1.0.0 |

## 打包目标

| 平台 | 格式 | 工具 |
|---|---|---|
| Windows | `.exe` NSIS 安装包 | CPack + NSIS |
| Ubuntu/Debian | `.deb` | CPack + dpkg |
| openKylin x86_64 | `.deb` | 同 Debian，需在 Kylin 环境构建验证 |
| macOS | `.dmg` | CPack + macOS 原生工具 |

## CMake CPack 配置示例

```cmake
set(CPACK_PACKAGE_NAME "FengSui")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR "FengSui")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Desktop-first LAN IM & File Collaboration")

if(WIN32)
  set(CPACK_GENERATOR "NSIS")
  set(CPACK_NSIS_DISPLAY_NAME "烽燧 FengSui")
elseif(APPLE)
  set(CPACK_GENERATOR "DragNDrop")
else()
  set(CPACK_GENERATOR "DEB")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "FengSui Team")
endif()

include(CPack)
```

## 各平台注意事项

### Windows
- 需安装 NSIS（`choco install nsis` 或手动安装）
- Qt DLL 通过 `windeployqt` 自动收集
- 建议附带 vcredist（Visual C++ Redistributable）

### Linux (Ubuntu/Debian)
- 依赖：`libqt6widgets6`、`libqt6network6`、`libqt6sql6-sqlite`
- `linuxdeployqt` 或手动 package

### openKylin
- 需核实 Qt6 软件包可用性（Kylin 软件源通常提供 Qt5，Qt6 可能需要自行编译或使用 AppImage）
- 如 Qt6 不可用，考虑 AppImage 打包方式作为备选

### macOS
- 使用 `macdeployqt` 收集 Qt 框架
- 签名（如需分发到非开发者机器）：`codesign`
- 公证（Notarization）：提交到 Apple（如需）

## 发布检查清单

- [ ] 版本号已更新（CMakeLists.txt 和 About 对话框）
- [ ] 四平台构建通过
- [ ] 安装包在干净虚拟机/系统上安装测试通过
- [ ] 首次启动向导正常
- [ ] 发现、消息、传输、共享基本流程正常
- [ ] 没有硬编码路径或调试日志泄露
- [ ] 崩溃转储收集机制已配置（Windows: `SetUnhandledExceptionFilter`；Linux: core dump）
