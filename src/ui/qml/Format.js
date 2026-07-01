// Format.js
// 纯函数格式化工具（字节数、传输方向/状态文案与配色）。以 .import 方式在 QML 中使用。
.pragma library

// 人类可读的字节数，如 1.5 MB
function bytes(n) {
    n = Number(n) || 0
    if (n < 1024) return n + " B"
    var units = ["KB", "MB", "GB", "TB"]
    var v = n / 1024
    var i = 0
    while (v >= 1024 && i < units.length - 1) { v /= 1024; i++ }
    return v.toFixed(v < 10 ? 1 : 0) + " " + units[i]
}

// 传输方向文案（0=Upload 我发出，1=Download 我接收）
function direction(dir) {
    return dir === 0 ? "发送" : "接收"
}

// 传输状态文案（对应 Enums.TransferStatus）
function statusText(st) {
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
