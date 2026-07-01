// ThemeController.h
// 主题控制器：暗色/亮色状态的唯一来源，供 QML 单例 Theme.qml 绑定。
// 支持三种模式：跟随系统 / 强制亮色 / 强制暗色，选择持久化到 AppSettings。

#pragma once

#include <QObject>
#include <QString>

namespace FengSui {

class AppSettings;

// 主题控制器。
// 由 main.cpp 构造（注入 AppSettings）并用 qmlRegisterSingletonInstance 注册给 QML，
// QML 中通过 ThemeController.isDark / ThemeController.mode 访问。
// System 模式下监听系统配色变化（QStyleHints::colorScheme），实时反映到 isDark。
class ThemeController : public QObject {
    Q_OBJECT

    // 当前是否为暗色（Theme.qml 依据此值在两套配色间切换）
    Q_PROPERTY(bool isDark READ isDark NOTIFY isDarkChanged)
    // 当前模式字符串："system" / "light" / "dark"
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged)

public:
    // 主题模式
    enum class Mode {
        System,  // 跟随系统
        Light,   // 强制亮色
        Dark     // 强制暗色
    };
    Q_ENUM(Mode)

    explicit ThemeController(QObject* parent = nullptr);

    // 注入设置以持久化主题选择并读取初始模式。
    // 应在 QML 加载前调用；内部据此确定初始 isDark 并连接系统配色信号。
    void setAppSettings(AppSettings* settings);

    bool isDark() const;

    QString mode() const;
    void setMode(const QString& mode);

signals:
    void isDarkChanged();
    void modeChanged();

private:
    // 依据当前模式与系统配色重新计算 isDark，变化则发信号
    void recomputeIsDark();

    // 系统当前是否为暗色（读取 QStyleHints::colorScheme）
    static bool systemPrefersDark();

    // 模式字符串与枚举互转
    static Mode modeFromString(const QString& s);
    static QString modeToString(Mode m);

    AppSettings* m_settings = nullptr;
    Mode         m_mode = Mode::System;
    bool         m_isDark = false;
};

} // namespace FengSui
