// AppIcon.qml
// 单色线性图标：内置一套 Feather/Lucide 风格的 24×24 描边 SVG 路径，
// 运行时按 name 取路径 + color 生成着色 SVG，交给 Image 渲染。可随主题变色，
// 矢量清晰、无第三方依赖。用法：AppIcon { name: "chat"; color: Theme.accent }

import QtQuick
import FengSui.Ui

Item {
    id: root

    property string name: ""
    property color color: Theme.text
    property int size: 20
    property real strokeWidth: 2

    implicitWidth: size
    implicitHeight: size

    // 图标名 → SVG 路径 d（均为 24×24 viewBox 的描边路径，无填充）
    readonly property var _paths: ({
        "chat":     "M21 11.5a8.38 8.38 0 0 1-.9 3.8 8.5 8.5 0 0 1-7.6 4.7 8.38 8.38 0 0 1-3.8-.9L3 21l1.9-5.7a8.38 8.38 0 0 1-.9-3.8 8.5 8.5 0 0 1 4.7-7.6 8.38 8.38 0 0 1 3.8-.9h.5a8.48 8.48 0 0 1 8 8v.5z",
        "contacts": "M17 21v-2a4 4 0 0 0-4-4H5a4 4 0 0 0-4 4v2 M9 11a4 4 0 1 0 0-8 4 4 0 0 0 0 8 M23 21v-2a4 4 0 0 0-3-3.87 M16 3.13a4 4 0 0 1 0 7.75",
        "transfer": "M17 1l4 4-4 4 M3 11V9a4 4 0 0 1 4-4h14 M7 23l-4-4 4-4 M21 13v2a4 4 0 0 1-4 4H3",
        "share":    "M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z",
        "settings": "M12 15a3 3 0 1 0 0-6 3 3 0 0 0 0 6z M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 1 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 1 1-2.83-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 1 1 2.83-2.83l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 1 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z",
        "plus":     "M12 5v14 M5 12h14",
        "search":   "M11 19a8 8 0 1 0 0-16 8 8 0 0 0 0 16z M21 21l-4.35-4.35",
        "send":     "M22 2L11 13 M22 2l-7 20-4-9-9-4 20-7z",
        "download": "M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4 M7 10l5 5 5-5 M12 15V3",
        "folder":   "M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z",
        "file":     "M13 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V9z M13 2v7h7",
        "check":    "M20 6L9 17l-5-5",
        "checkDouble": "M18 6L7 17l-3-3 M22 10l-7 7-1-1",
        "clock":    "M12 22a10 10 0 1 0 0-20 10 10 0 0 0 0 20z M12 6v6l4 2",
        "x":        "M18 6L6 18 M6 6l12 12",
        "moon":     "M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z",
        "sun":      "M12 17a5 5 0 1 0 0-10 5 5 0 0 0 0 10z M12 1v2 M12 21v2 M4.22 4.22l1.42 1.42 M18.36 18.36l1.42 1.42 M1 12h2 M21 12h2 M4.22 19.78l1.42-1.42 M18.36 5.64l1.42-1.42",
        "chevronRight": "M9 18l6-6-6-6",
        "chevronDown": "M6 9l6 6 6-6",
        "arrowUp":  "M12 19V5 M5 12l7-7 7 7",
        "arrowDown":"M12 5v14 M19 12l-7 7-7-7",
        "trash":    "M3 6h18 M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2",
        "refresh":  "M23 4v6h-6 M1 20v-6h6 M20.49 9A9 9 0 0 0 5.64 5.64L1 10m22 4l-4.64 4.36A9 9 0 0 1 3.51 15",
        "alert":    "M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z M12 9v4 M12 17h.01"
    })

    // 生成着色 SVG（stroke 用 color，无填充，圆头圆角）
    readonly property string _svg: {
        var d = _paths[name]
        if (!d)
            return ""
        var c = root.color
        var hex = "#%1%2%3"
            .arg(Math.round(c.r * 255).toString(16).padStart(2, "0"))
            .arg(Math.round(c.g * 255).toString(16).padStart(2, "0"))
            .arg(Math.round(c.b * 255).toString(16).padStart(2, "0"))
        return "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' " +
               "fill='none' stroke='" + hex + "' stroke-width='" + root.strokeWidth +
               "' stroke-linecap='round' stroke-linejoin='round'>" +
               "<path d='" + d + "'/></svg>"
    }

    Image {
        anchors.fill: parent
        source: root._svg.length > 0
                ? "data:image/svg+xml;utf8," + encodeURIComponent(root._svg)
                : ""
        sourceSize.width: root.size * 2      // 2x 采样保证清晰
        sourceSize.height: root.size * 2
        smooth: true
        fillMode: Image.PreserveAspectFit
    }
}
