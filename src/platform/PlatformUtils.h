// PlatformUtils.h
// 跨平台工具函数：平台检测、主机名获取、应用数据路径等。
// 后续任务中按需补充。

#pragma once

#include <QString>

namespace FengSui {

// 获取当前操作系统标识："windows" / "linux" / "macos"
QString platformOs();

// 获取系统主机名
QString hostName();

} // namespace FengSui
