// PlatformUtils.cpp
// 跨平台工具实现。

#include "platform/PlatformUtils.h"

#include <QHostInfo>

#if defined(Q_OS_WIN)
#  define FENGSUI_OS "windows"
#elif defined(Q_OS_MACOS)
#  define FENGSUI_OS "macos"
#else
#  define FENGSUI_OS "linux"
#endif

namespace FengSui {

QString platformOs()
{
    return QString::fromUtf8(FENGSUI_OS);
}

QString hostName()
{
    return QHostInfo::localHostName();
}

} // namespace FengSui
