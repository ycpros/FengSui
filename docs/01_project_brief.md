# 01 — 项目总览 (Project Brief)

## 项目名称

**烽燧（FengSui）内联协作系统**

## 一句话定位

桌面端优先、无外网可用、默认无中心服务器的同网段轻量 IM 与文件协作工具。

## 目标用户

- 断网研发/测试团队（研发网、保密网、实验室网）
- 办公室行政/财务（同网段 Windows 为主）
- 设计/视频/教务资料团队（Win + Mac + Linux 混用）
- 外协/访客协作（临时来访设备，不便装企业 IM）
- 小型私有化客户（10–100 人内网）

## 核心场景

1. 两台同网段电脑装上即可互发消息、传文件，无需任何服务端
2. 共享本地目录给其他人浏览下载，替代 SMB/FTP/NAS 配置
3. 创建投递箱让协作者提交文件，自动统计
4. 广播通知全网段成员并要求确认收到
5. 网络不通时内置诊断工具自助排查

## MVP 范围

**P0（首发必做）：** 设备发现、单聊文本、文件/文件夹传输、共享文件夹浏览、首次访问授权、传输中心、网络诊断、四平台打包（Windows / Ubuntu/Debian / openKylin x86_64 / macOS）

**P1（后续）：** 广播通知确认、文件投递箱、断点续传、浏览器访客模式、托盘增强

## 明确非目标

- 不做音视频会议
- 不做在线文档协作
- 不做组织架构/审批/OA
- 不做互联网跨网远程传输
- 不做真正的云离线消息
- MVP 不做跨 VLAN/跨子网发现
- 不做移动端（Android/iOS）

## 平台范围

Windows、Ubuntu/Debian 系、openKylin x86_64、macOS。

## 技术路线（当前阶段）

**V1.0：纯 Qt 6 + C++ 单进程**

- UI: Qt 6 Widgets
- 网络: Qt Network (TCP/UDP)
- 存储: SQLite（本地嵌入式）
- 构建: CMake + CPack

**明确不引入的技术：** Rust、Asio、Electron、Flutter、QML、任何 Web 前端框架。

## 当前阶段目标

从零搭建 MVP 原型，输出可安装运行的桌面客户端，完成 P0 全部功能的闭环验证。

## 你的第一步

1. 阅读本文件了解项目全貌
2. 阅读 [03_mvp_scope.md](./03_mvp_scope.md) 理解具体做什么
3. 阅读 [04_architecture.md](./04_architecture.md) 理解怎么做
4. 阅读 [12_agent_instructions.md](./12_agent_instructions.md) 理解行为规则
5. 按 [09_task_breakdown.md](./09_task_breakdown.md) 顺序逐任务实施
