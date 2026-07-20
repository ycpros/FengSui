// Format.mjs
// 纯函数格式化工具，提供字节数、传输方向和传输状态文案。
// 文件使用标准 ECMAScript Module 显式导出；QML 调用方通过
// `import "../qml/Format.mjs" as Format` 引入，避免旧式脚本隐式全局符号。

// 将字节数转换为人类可读文本。
// n: 字节数；无法转换为数字的输入按 0 处理。
// 返回值：B、KB、MB、GB 或 TB 文本，例如 "1.5 MB"。
export function bytes(n) {
    n = Number(n) || 0
    if (n < 1024) return n + " B"
    var units = ["KB", "MB", "GB", "TB"]
    var v = n / 1024
    var i = 0
    while (v >= 1024 && i < units.length - 1) { v /= 1024; i++ }
    return v.toFixed(v < 10 ? 1 : 0) + " " + units[i]
}

// 返回传输方向文案。
// dir: Enums.TransferDirection 数值，0 表示 Upload，其他值按 Download 展示。
// 返回值：发送方视角的“发送”或“接收”。
export function direction(dir) {
    return dir === 0 ? "发送" : "接收"
}

// 返回传输状态文案。
// st: Enums.TransferStatus 数值，范围为 0 到 5。
// 返回值：对应的中文状态；未知枚举值返回“未知”。
export function statusText(st) {
    switch (st) {
    case 0: return "等待中"    // Waiting
    case 1: return "已拒绝"    // Rejected
    case 2: return "传输中"    // Transferring
    case 3: return "已完成"    // Completed
    case 4: return "失败"      // Failed
    case 5: return "已取消"    // Cancelled
    default: return "未知"
    }
}
