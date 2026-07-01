// ThemeController.cpp
// 主题控制器实现。

#include "ui/viewmodels/ThemeController.h"

#include "app/AppSettings.h"

#include <QGuiApplication>
#include <QStyleHints>

namespace FengSui {

ThemeController::ThemeController(QObject* parent)
    : QObject(parent)
{
    // 监听系统配色变化：仅在 System 模式下才影响 isDark，但信号始终连接，
    // 由 recomputeIsDark 根据当前模式决定是否采用系统值。
    if (auto* hints = QGuiApplication::styleHints()) {
        connect(hints, &QStyleHints::colorSchemeChanged,
                this, [this](Qt::ColorScheme) { recomputeIsDark(); });
    }
    recomputeIsDark();
}

void ThemeController::setAppSettings(AppSettings* settings)
{
    m_settings = settings;
    if (m_settings) {
        m_mode = modeFromString(m_settings->themeMode());
        emit modeChanged();
    }
    recomputeIsDark();
}

bool ThemeController::isDark() const
{
    return m_isDark;
}

QString ThemeController::mode() const
{
    return modeToString(m_mode);
}

void ThemeController::setMode(const QString& mode)
{
    const Mode next = modeFromString(mode);
    if (next == m_mode) {
        return;
    }
    m_mode = next;
    if (m_settings) {
        m_settings->setThemeMode(modeToString(m_mode));
    }
    emit modeChanged();
    recomputeIsDark();
}

void ThemeController::recomputeIsDark()
{
    bool dark = false;
    switch (m_mode) {
    case Mode::System:
        dark = systemPrefersDark();
        break;
    case Mode::Light:
        dark = false;
        break;
    case Mode::Dark:
        dark = true;
        break;
    }

    if (dark != m_isDark) {
        m_isDark = dark;
        emit isDarkChanged();
    }
}

bool ThemeController::systemPrefersDark()
{
    // Qt 6.5+ 通过 QStyleHints::colorScheme 上报系统配色。
    // Linux（openKylin/Debian 等）依赖桌面 portal，可能返回 Unknown，此时回退亮色。
    if (auto* hints = QGuiApplication::styleHints()) {
        return hints->colorScheme() == Qt::ColorScheme::Dark;
    }
    return false;
}

ThemeController::Mode ThemeController::modeFromString(const QString& s)
{
    const QString v = s.trimmed().toLower();
    if (v == QLatin1String("light")) {
        return Mode::Light;
    }
    if (v == QLatin1String("dark")) {
        return Mode::Dark;
    }
    return Mode::System;
}

QString ThemeController::modeToString(Mode m)
{
    switch (m) {
    case Mode::Light: return QStringLiteral("light");
    case Mode::Dark:  return QStringLiteral("dark");
    case Mode::System:
    default:          return QStringLiteral("system");
    }
}

} // namespace FengSui
