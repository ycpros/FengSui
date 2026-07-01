// Theme.qml
// 全局设计令牌单例：颜色、间距、圆角、字号、动画时长集中管理。
// 暗色/亮色由 ThemeController.isDark 驱动，切换只改一个布尔，全 UI 随三元表达式重算。
// 视觉参考 Telegram Desktop + Linear + Raycast，暗色优先。

pragma Singleton

import QtQuick
import FengSui.Ui

QtObject {
    id: theme

    // 是否暗色，来源于 C++ 主题控制器
    readonly property bool dark: ThemeController.isDark

    // ---- 颜色 ----
    // 背景层级：bg（最底）< surface < surfaceAlt（浮起卡片/悬停）
    readonly property color bg:         dark ? "#17181c" : "#ffffff"
    readonly property color surface:    dark ? "#1f2126" : "#f7f8fa"
    readonly property color surfaceAlt: dark ? "#282b31" : "#eef0f3"
    readonly property color border:     dark ? "#31343b" : "#e2e5ea"
    readonly property color overlay:    dark ? "#000000cc" : "#00000066"  // 模态遮罩

    // 文本（text 最深 > textSecondary > textMuted > textFaint）
    readonly property color text:          dark ? "#e8e9eb" : "#1c1d21"
    readonly property color textSecondary: dark ? "#c3c6cd" : "#3d4250"
    readonly property color textMuted:     dark ? "#9497a1" : "#667085"
    readonly property color textFaint:     dark ? "#6b6e78" : "#98a1b0"

    // 强调色（品牌青绿，取自旧 UI 主色，两态微调保证对比度）
    readonly property color accent:      dark ? "#2dd4bf" : "#147d7e"
    readonly property color accentHover: dark ? "#5eead4" : "#0f6364"
    readonly property color accentText:  "#ffffff"                        // 强调底上的文字

    // 聊天气泡
    readonly property color bubbleOut:      dark ? "#155e63" : "#147d7e"  // 自己发出（靠右）
    readonly property color bubbleOutText:  "#ffffff"
    readonly property color bubbleIn:       dark ? "#282b31" : "#ffffff"  // 对方（靠左）
    readonly property color bubbleInText:   dark ? "#e8e9eb" : "#1c1d21"
    readonly property color bubbleInBorder: dark ? "#31343b" : "#e2e5ea"

    // 语义色
    readonly property color online:  "#22c55e"
    readonly property color offline: dark ? "#6b6e78" : "#98a1b0"
    readonly property color danger:  dark ? "#f87171" : "#dc2626"
    readonly property color warning: dark ? "#fbbf24" : "#d97706"
    readonly property color info:    dark ? "#60a5fa" : "#2563eb"

    // ---- 间距 ----
    readonly property int spaceXs: 4
    readonly property int spaceSm: 8
    readonly property int spaceMd: 16
    readonly property int spaceLg: 24
    readonly property int spaceXl: 32
    readonly property int navWidth: 260

    // ---- 圆角 ----
    readonly property int radiusSm: 6
    readonly property int radiusMd: 10
    readonly property int radiusLg: 16
    readonly property int radiusPill: 999

    // ---- 字号 ----
    readonly property int fontXs: 11
    readonly property int fontSm: 13
    readonly property int fontMd: 15
    readonly property int fontLg: 18
    readonly property int fontXl: 24

    // ---- 字体 ----
    // 空字符串走系统默认 CJK（Windows 微软雅黑 / Linux Noto Sans CJK）
    readonly property string fontFamily: ""
    readonly property string monoFamily: "Consolas, 'Cascadia Mono', 'Courier New', monospace"

    // ---- 动画时长（毫秒）----
    readonly property int animFast: 120
    readonly property int animMed:  200
    readonly property int animSlow: 320
}
