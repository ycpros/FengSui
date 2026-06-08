const fs = require('fs');
const path = require('path');
let sharp = null;
try {
  sharp = require('sharp');
} catch (error) {
  sharp = null;
}

const outDir = path.join(__dirname, 'screens');
const W = 1280;
const H = 820;

const C = {
  page: '#eef2f4',
  ink: '#1f2933',
  text: '#263238',
  muted: '#66727c',
  faint: '#8b98a5',
  line: '#d8dee3',
  softLine: '#e8ecef',
  panel: '#ffffff',
  panel2: '#f7f9fa',
  sidebar: '#20262d',
  sidebar2: '#2b323a',
  navText: '#d7dde2',
  navMuted: '#9aa6b2',
  accent: '#0f8f8c',
  accent2: '#1f9d72',
  accentSoft: '#e3f4f2',
  green: '#1f9d72',
  greenSoft: '#e8f6ef',
  blue: '#3478b9',
  blueSoft: '#e8f1fa',
  amber: '#b7791f',
  amberSoft: '#fff4db',
  red: '#cf3f3f',
  redSoft: '#fdeaea',
  purple: '#6b5fb5',
  purpleSoft: '#efedfa',
  shadow: 'rgba(31,41,51,.15)',
};

const nav = [
  ['messages', '消息'],
  ['contacts', '联系人'],
  ['transfer', '传输中心'],
  ['share', '共享文件'],
  ['dropbox', '投递箱'],
  ['settings', '设置'],
];

const screens = [
  {
    file: '01_onboarding.svg',
    title: '01 首次启动向导',
    subtitle: '三步完成昵称、网络发现与默认下载目录设置。',
    priority: 'P0 必做',
    source: '源码已落地',
    draw: drawOnboarding,
  },
  {
    file: '02_main_messages.svg',
    title: '02 主界面 - 消息与会话',
    subtitle: '默认入口：左侧导航、会话列表与聊天区域。',
    priority: 'P0 必做',
    source: '源码已落地',
    draw: (s) => drawMainMessages(s),
  },
  {
    file: '03_contacts_manual_ip.svg',
    title: '03 联系人 - 自动发现与手动 IP',
    subtitle: '展示在线设备、系统类型、连接端点与手动添加兜底路径。',
    priority: 'P0 必做',
    source: '源码已落地',
    draw: drawContacts,
  },
  {
    file: '03_chat_file_send.svg',
    title: '04 单聊 - 文件拖拽发送',
    subtitle: '聊天上下文中发文本、拖文件、处理接收确认与传输状态。',
    priority: 'P0 必做',
    source: '部分占位',
    draw: drawChatFile,
  },
  {
    file: '04_broadcast_confirm.svg',
    title: '05 广播通知 - 确认收到',
    subtitle: '向全网段发送通知，并查看已确认与未确认列表。',
    priority: 'P1 规划',
    source: '规划原型',
    draw: drawBroadcast,
  },
  {
    file: '05_transfer_center.svg',
    title: '06 传输中心',
    subtitle: '统一查看发送、接收、下载、失败与重试状态。',
    priority: 'P0 必做',
    source: '源码占位',
    draw: drawTransferCenter,
  },
  {
    file: '06_online_shares.svg',
    title: '07 共享文件 - 浏览在线共享',
    subtitle: '选择在线共享源，浏览目录并下载文件。',
    priority: 'P0 必做',
    source: '源码占位',
    draw: drawOnlineShares,
  },
  {
    file: '07_my_shares.svg',
    title: '08 我的共享 - 目录管理',
    subtitle: '添加、启用、停用或移除本机共享目录。',
    priority: 'P0 必做',
    source: '源码占位',
    draw: drawMyShares,
  },
  {
    file: '08_permission_modal.svg',
    title: '09 授权弹窗 - 首次访问确认',
    subtitle: '首次访问共享目录时，由共享方明确允许、拒绝或记住选择。',
    priority: 'P0 必做',
    source: '待实现',
    draw: drawPermissionModal,
  },
  {
    file: '09_dropbox_list.svg',
    title: '10 文件投递箱 - 任务列表',
    subtitle: 'P1 资料收集入口：任务、截止时间、提交统计。',
    priority: 'P1 规划',
    source: '源码占位',
    draw: drawDropboxList,
  },
  {
    file: '10_create_dropbox.svg',
    title: '11 新建投递任务',
    subtitle: '创建文件收集任务并设置说明、截止时间与可见范围。',
    priority: 'P1 规划',
    source: '源码占位',
    draw: drawCreateDropbox,
  },
  {
    file: '11_settings_network.svg',
    title: '12 设置 - 网络发现',
    subtitle: '常规设置、发现开关、监听端口与手动连接入口。',
    priority: 'P0 必做',
    source: '源码占位',
    draw: drawSettings,
  },
  {
    file: '12_network_diagnostics.svg',
    title: '13 网络诊断',
    subtitle: '一屏排查本机 IP、端口、防火墙、UDP 广播与在线设备数。',
    priority: 'P0 必做',
    source: '源码占位',
    draw: drawDiagnostics,
  },
  {
    file: '13_tray_menu.svg',
    title: '14 系统托盘菜单',
    subtitle: '关闭主窗口后的托盘入口、在线状态切换与退出流程。',
    priority: 'P1 规划',
    source: '源码存在',
    draw: drawTray,
  },
  {
    file: '14_information_architecture.svg',
    title: '15 信息架构与流程总览',
    subtitle: '用于评审 P0/P1 页面关系、核心链路和版本边界。',
    priority: '总览',
    source: '设计说明',
    draw: drawInformationArchitecture,
  },
];

function esc(value) {
  return String(value)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

function svgWrap(inner) {
  return `<svg width="${W}" height="${H}" viewBox="0 0 ${W} ${H}" xmlns="http://www.w3.org/2000/svg">
  <defs>
    <filter id="softShadow" x="-20%" y="-20%" width="140%" height="140%">
      <feDropShadow dx="0" dy="18" stdDeviation="18" flood-color="#1f2933" flood-opacity="0.14"/>
    </filter>
    <filter id="tinyShadow" x="-20%" y="-20%" width="140%" height="140%">
      <feDropShadow dx="0" dy="4" stdDeviation="6" flood-color="#1f2933" flood-opacity="0.10"/>
    </filter>
    <linearGradient id="windowTop" x1="0" x2="1" y1="0" y2="0">
      <stop offset="0" stop-color="#f9fbfc"/>
      <stop offset="1" stop-color="#eef3f5"/>
    </linearGradient>
    <marker id="arrow" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="8" markerHeight="8" orient="auto-start-reverse">
      <path d="M 0 0 L 10 5 L 0 10 z" fill="${C.faint}" />
    </marker>
  </defs>
  <rect width="${W}" height="${H}" fill="${C.page}"/>
  ${inner}
</svg>`;
}

function rect(x, y, w, h, r, fill = C.panel, stroke = 'none', sw = 1, extra = '') {
  return `<rect x="${x}" y="${y}" width="${w}" height="${h}" rx="${r}" fill="${fill}" stroke="${stroke}" stroke-width="${sw}" ${extra}/>`;
}

function circle(x, y, r, fill = C.panel, stroke = 'none', sw = 1) {
  return `<circle cx="${x}" cy="${y}" r="${r}" fill="${fill}" stroke="${stroke}" stroke-width="${sw}"/>`;
}

function line(x1, y1, x2, y2, stroke = C.line, sw = 1, extra = '') {
  return `<line x1="${x1}" y1="${y1}" x2="${x2}" y2="${y2}" stroke="${stroke}" stroke-width="${sw}" ${extra}/>`;
}

function svgPath(d, stroke = C.text, sw = 2, fill = 'none', extra = '') {
  return `<path d="${d}" fill="${fill}" stroke="${stroke}" stroke-width="${sw}" stroke-linecap="round" stroke-linejoin="round" ${extra}/>`;
}

function text(x, y, value, size = 14, weight = 400, fill = C.text, anchor = 'start', extra = '') {
  return `<text x="${x}" y="${y}" font-family="Inter, -apple-system, BlinkMacSystemFont, Segoe UI, Microsoft YaHei, PingFang SC, Noto Sans CJK SC, Arial, sans-serif" font-size="${size}" font-weight="${weight}" fill="${fill}" text-anchor="${anchor}" ${extra}>${esc(value)}</text>`;
}

function multiLine(x, y, lines, size = 13, gap = 22, fill = C.muted, weight = 400) {
  return lines.map((value, i) => text(x, y + i * gap, value, size, weight, fill)).join('');
}

function pill(x, y, label, opts = {}) {
  const pad = opts.pad ?? 13;
  const size = opts.size ?? 13;
  const w = opts.width ?? Math.max(52, label.length * size * 0.88 + pad * 2);
  const h = opts.height ?? 28;
  return `${rect(x, y, w, h, h / 2, opts.fill ?? C.panel2, opts.stroke ?? C.line)}
    ${text(x + w / 2, y + h / 2 + size * 0.36, label, size, opts.weight ?? 600, opts.color ?? C.text, 'middle')}`;
}

function button(x, y, w, h, label, variant = 'primary', iconName = null) {
  const map = {
    primary: [C.accent, C.accent, '#fff'],
    dark: [C.sidebar, C.sidebar, '#fff'],
    ghost: [C.panel, C.line, C.text],
    soft: [C.accentSoft, '#b8deda', C.accent],
    danger: [C.redSoft, '#f3b5b5', C.red],
  };
  const [fill, stroke, color] = map[variant] ?? map.primary;
  const iconSvg = iconName ? icon(iconName, x + 13, y + (h - 18) / 2, 18, color) : '';
  const tx = iconName ? x + 38 : x + w / 2;
  const anchor = iconName ? 'start' : 'middle';
  return `${rect(x, y, w, h, 6, fill, stroke)}
    ${iconSvg}
    ${text(tx, y + h / 2 + 5, label, 13, 650, color, anchor)}`;
}

function icon(name, x, y, size = 20, color = C.text) {
  const s = size;
  const cx = x + s / 2;
  const cy = y + s / 2;
  const p = [];
  const l = (x1, y1, x2, y2) => p.push(line(x + x1 * s, y + y1 * s, x + x2 * s, y + y2 * s, color, 1.8));
  const c = (rx, ry, rr) => p.push(circle(x + rx * s, y + ry * s, rr * s, 'none', color, 1.8));
  const r = (rx, ry, rw, rh, rr = 0.12) => p.push(rect(x + rx * s, y + ry * s, rw * s, rh * s, rr * s, 'none', color, 1.8));
  switch (name) {
    case 'message':
      r(.16, .2, .68, .52, .12); l(.28, .72, .2, .9, .42, .72); break;
    case 'users':
      c(.38, .34, .16); c(.68, .38, .12); p.push(svgPath(`M ${x+s*.12} ${y+s*.82} C ${x+s*.2} ${y+s*.6} ${x+s*.56} ${y+s*.6} ${x+s*.64} ${y+s*.82}`, color, 1.8)); p.push(svgPath(`M ${x+s*.57} ${y+s*.76} C ${x+s*.64} ${y+s*.62} ${x+s*.84} ${y+s*.64} ${x+s*.9} ${y+s*.8}`, color, 1.8)); break;
    case 'transfer':
      l(.2, .34, .76, .34); l(.58, .18, .76, .34); l(.58, .5, .76, .34); l(.8, .66, .24, .66); l(.42, .5, .24, .66); l(.42, .82, .24, .66); break;
    case 'folder':
      p.push(svgPath(`M ${x+s*.12} ${y+s*.34} L ${x+s*.38} ${y+s*.34} L ${x+s*.46} ${y+s*.24} L ${x+s*.88} ${y+s*.24} L ${x+s*.88} ${y+s*.78} L ${x+s*.12} ${y+s*.78} Z`, color, 1.8)); break;
    case 'inbox':
      r(.16, .18, .68, .64, .1); l(.28, .53, .42, .68); l(.42, .68, .58, .68); l(.58, .68, .72, .53); break;
    case 'settings':
      c(.5, .5, .18); p.push(svgPath(`M ${cx} ${y+s*.12} V ${y+s*.25} M ${cx} ${y+s*.75} V ${y+s*.88} M ${x+s*.12} ${cy} H ${x+s*.25} M ${x+s*.75} ${cy} H ${x+s*.88} M ${x+s*.23} ${y+s*.23} L ${x+s*.32} ${y+s*.32} M ${x+s*.68} ${y+s*.68} L ${x+s*.77} ${y+s*.77} M ${x+s*.77} ${y+s*.23} L ${x+s*.68} ${y+s*.32} M ${x+s*.32} ${y+s*.68} L ${x+s*.23} ${y+s*.77}`, color, 1.7)); break;
    case 'search':
      c(.43, .43, .24); l(.62, .62, .84, .84); break;
    case 'plus':
      l(.5, .2, .5, .8); l(.2, .5, .8, .5); break;
    case 'send':
      p.push(svgPath(`M ${x+s*.15} ${y+s*.18} L ${x+s*.86} ${y+s*.5} L ${x+s*.15} ${y+s*.82} L ${x+s*.28} ${y+s*.52} L ${x+s*.54} ${y+s*.5} L ${x+s*.28} ${y+s*.48} Z`, color, 1.6)); break;
    case 'file':
      p.push(svgPath(`M ${x+s*.24} ${y+s*.12} H ${x+s*.6} L ${x+s*.78} ${y+s*.3} V ${y+s*.88} H ${x+s*.24} Z M ${x+s*.6} ${y+s*.12} V ${y+s*.3} H ${x+s*.78}`, color, 1.7)); break;
    case 'check':
      p.push(svgPath(`M ${x+s*.2} ${y+s*.54} L ${x+s*.42} ${y+s*.74} L ${x+s*.82} ${y+s*.28}`, color, 2.2)); break;
    case 'x':
      l(.24, .24, .76, .76); l(.76, .24, .24, .76); break;
    case 'wifi':
      p.push(svgPath(`M ${x+s*.16} ${y+s*.38} C ${x+s*.36} ${y+s*.2} ${x+s*.64} ${y+s*.2} ${x+s*.84} ${y+s*.38} M ${x+s*.28} ${y+s*.52} C ${x+s*.4} ${y+s*.42} ${x+s*.6} ${y+s*.42} ${x+s*.72} ${y+s*.52} M ${x+s*.42} ${y+s*.66} C ${x+s*.47} ${y+s*.62} ${x+s*.53} ${y+s*.62} ${x+s*.58} ${y+s*.66}`, color, 1.9)); p.push(circle(cx, y+s*.78, s*.035, color, color)); break;
    case 'shield':
      p.push(svgPath(`M ${cx} ${y+s*.1} L ${x+s*.82} ${y+s*.24} V ${y+s*.46} C ${x+s*.82} ${y+s*.66} ${cx} ${y+s*.86} ${cx} ${y+s*.86} C ${cx} ${y+s*.86} ${x+s*.18} ${y+s*.66} ${x+s*.18} ${y+s*.46} V ${y+s*.24} Z`, color, 1.8)); break;
    case 'bell':
      p.push(svgPath(`M ${x+s*.28} ${y+s*.66} H ${x+s*.72} C ${x+s*.66} ${y+s*.58} ${x+s*.64} ${y+s*.48} ${x+s*.64} ${y+s*.36} C ${x+s*.64} ${y+s*.22} ${x+s*.54} ${y+s*.16} ${cx} ${y+s*.16} C ${x+s*.46} ${y+s*.16} ${x+s*.36} ${y+s*.22} ${x+s*.36} ${y+s*.36} C ${x+s*.36} ${y+s*.48} ${x+s*.34} ${y+s*.58} ${x+s*.28} ${y+s*.66} Z M ${x+s*.44} ${y+s*.74} C ${x+s*.47} ${y+s*.82} ${x+s*.53} ${y+s*.82} ${x+s*.56} ${y+s*.74}`, color, 1.7)); break;
    case 'download':
      l(.5, .18, .5, .62); l(.32, .45, .5, .64); l(.68, .45, .5, .64); l(.24, .82, .76, .82); break;
    case 'trash':
      l(.24, .3, .76, .3); l(.4, .2, .6, .2); r(.3, .3, .4, .54, .05); break;
    default:
      c(.5, .5, .32);
  }
  return `<g>${p.join('')}</g>`;
}

function statusDot(x, y, color = C.green) {
  return `${circle(x, y, 4, color)}${circle(x, y, 8, color, color, 1).replace('/>', ' opacity=".14"/>')}`;
}

function canvasHeader(s) {
  const x = 42;
  return `
    ${text(x, 52, s.title, 24, 750, C.ink)}
    ${text(x, 80, s.subtitle, 13, 400, C.muted)}
    ${pill(914, 35, s.priority, { fill: tagFill(s.priority), stroke: tagStroke(s.priority), color: tagColor(s.priority), width: 110 })}
    ${pill(1038, 35, s.source, { fill: C.panel, stroke: C.line, color: C.muted, width: 116 })}
    ${pill(1168, 35, '高保真', { fill: C.sidebar, stroke: C.sidebar, color: '#fff', width: 72 })}
  `;
}

function tagFill(label) {
  if (label.includes('P1')) return C.purpleSoft;
  if (label.includes('总览')) return C.amberSoft;
  return C.greenSoft;
}

function tagStroke(label) {
  if (label.includes('P1')) return '#d5cef5';
  if (label.includes('总览')) return '#f3d38a';
  return '#bee6d3';
}

function tagColor(label) {
  if (label.includes('P1')) return C.purple;
  if (label.includes('总览')) return C.amber;
  return '#147b57';
}

function appShell(active = 'messages', content = '', options = {}) {
  const x = 42;
  const y = 104;
  const w = 1196;
  const h = 674;
  const sideW = 260;
  const topH = 54;
  const activeNav = active;
  const includeDropbox = options.includeDropbox ?? true;
  const navItems = includeDropbox ? nav : nav.filter(([key]) => key !== 'dropbox');
  let out = `
    ${rect(x, y, w, h, 12, C.panel, C.line, 1, 'filter="url(#softShadow)"')}
    ${rect(x, y, w, topH, 12, 'url(#windowTop)', C.line)}
    ${line(x, y + topH, x + w, y + topH, C.line)}
    ${circle(x + 22, y + 26, 6, '#ef5f5f')}
    ${circle(x + 42, y + 26, 6, '#e6b64c')}
    ${circle(x + 62, y + 26, 6, '#55b66a')}
    ${text(x + 86, y + 33, '烽燧 FengSui', 14, 720, C.ink)}
    ${rect(x + 356, y + 12, 310, 30, 6, '#fff', C.line)}
    ${icon('search', x + 368, y + 17, 18, C.faint)}
    ${text(x + 394, y + 32, '搜索联系人 / 文件 / 消息', 12, 400, C.faint)}
    ${statusDot(x + w - 228, y + 27, C.green)}
    ${text(x + w - 214, y + 32, 'Lily-PC 在线', 12, 650, C.green)}
    ${icon('bell', x + w - 122, y + 17, 18, C.muted)}
    ${pill(x + w - 88, y + 13, '12 台在线', { width: 72, height: 28, size: 12, fill: C.accentSoft, stroke: '#c2e4df', color: C.accent })}
    ${rect(x, y + topH, sideW, h - topH, 0, C.sidebar)}
    ${rect(x, y + h - 16, sideW, 16, 0, C.sidebar)}
    ${text(x + 28, y + 100, '本机', 11, 600, C.navMuted)}
    ${text(x + 28, y + 126, 'Lily-PC', 18, 760, '#fff')}
    ${text(x + 28, y + 148, '研发网 · 192.168.10.24', 12, 400, C.navMuted)}
  `;
  let ny = y + 184;
  for (const [key, label] of navItems) {
    const selected = key === activeNav;
    out += selected
      ? `${rect(x + 16, ny - 23, sideW - 32, 42, 6, '#ffffff', 'none')}`
      : `${rect(x + 16, ny - 23, sideW - 32, 42, 6, C.sidebar, 'none')}`;
    out += icon(navIcon(key), x + 34, ny - 14, 18, selected ? C.accent : C.navText);
    out += text(x + 64, ny, label, 14, selected ? 760 : 500, selected ? C.ink : C.navText);
    if (key === 'messages') out += pill(x + 204, ny - 18, '3', { width: 28, height: 22, size: 11, fill: selected ? C.redSoft : '#393f47', stroke: selected ? '#f4c0c0' : '#393f47', color: selected ? C.red : '#fff' });
    ny += 48;
  }
  out += `
    ${line(x + 24, y + h - 112, x + sideW - 24, y + h - 112, '#3b424b')}
    ${text(x + 28, y + h - 78, '发现方式', 11, 600, C.navMuted)}
    ${text(x + 28, y + h - 53, '自动广播 + 手动 IP', 13, 500, C.navText)}
    ${text(x + 28, y + h - 28, '监听端口 8787', 12, 400, C.navMuted)}
    ${content}
  `;
  return out;
}

function navIcon(key) {
  return ({
    messages: 'message',
    contacts: 'users',
    transfer: 'transfer',
    share: 'folder',
    dropbox: 'inbox',
    settings: 'settings',
  })[key] ?? 'message';
}

function sectionTitle(x, y, title, subtitle) {
  return `${text(x, y, title, 22, 760, C.ink)}${subtitle ? text(x, y + 26, subtitle, 12, 400, C.muted) : ''}`;
}

function card(x, y, w, h, r = 8, fill = C.panel) {
  return rect(x, y, w, h, r, fill, C.line);
}

function avatar(x, y, label, fill = C.blueSoft, color = C.blue) {
  return `${circle(x, y, 22, fill, '#d8e5f2')}${text(x, y + 6, label, 15, 760, color, 'middle')}`;
}

function tableHeader(x, y, w, columns) {
  let out = rect(x, y, w, 42, 6, C.panel2, C.softLine);
  columns.forEach(([label, cx]) => {
    out += text(x + cx, y + 27, label, 12, 720, C.muted);
  });
  return out;
}

function rowBg(x, y, w, h, i = 0) {
  return rect(x, y, w, h, 6, i % 2 ? C.panel : '#fbfcfd', C.softLine);
}

function progress(x, y, w, pct, color = C.green) {
  return `${rect(x, y, w, 8, 4, '#e5eaee')}${rect(x, y, Math.max(8, Math.round(w * pct)), 8, 4, color)}`;
}

function toggle(x, y, on = true) {
  return `${rect(x, y, 46, 24, 12, on ? C.accent : '#cdd5dc', 'none')}
    ${circle(x + (on ? 34 : 12), y + 12, 9, '#fff', 'rgba(31,41,51,.12)')}`;
}

function drawOnboarding(s) {
  const x = 250, y = 132, w = 780, h = 560;
  const steps = [
    ['1', '设备命名', true],
    ['2', '网络发现', true],
    ['3', '下载目录', true],
  ];
  let out = canvasHeader(s);
  out += `${rect(x, y, w, h, 12, C.panel, C.line, 1, 'filter="url(#softShadow)"')}
    ${rect(x, y, 248, h, 12, '#f4f8f7', C.line)}
    ${text(x + 36, y + 58, '烽燧 FengSui', 24, 780, C.ink)}
    ${multiLine(x + 36, y + 94, ['首次运行只需完成三项设置。', '其他设备会通过昵称识别你，', '默认目录用于保存收到的文件。'], 13, 22, C.muted)}
    ${rect(x + 36, y + 210, 176, 124, 8, '#fff', C.line)}
    ${icon('wifi', x + 62, y + 236, 30, C.accent)}
    ${text(x + 104, y + 255, '同网段发现', 14, 760, C.ink)}
    ${text(x + 104, y + 278, '5 秒内互见设备', 12, 400, C.muted)}
    ${rect(x + 36, y + 356, 176, 124, 8, '#fff', C.line)}
    ${icon('folder', x + 62, y + 382, 30, C.amber)}
    ${text(x + 104, y + 401, '默认下载', 14, 760, C.ink)}
    ${text(x + 104, y + 424, '接收文件可追踪', 12, 400, C.muted)}
    ${text(x + 286, y + 58, '首次设置', 26, 780, C.ink)}
    ${text(x + 286, y + 84, '完成后写入 SQLite，下次启动直接进入主窗口。', 13, 400, C.muted)}
  `;
  let sx = x + 286;
  steps.forEach(([num, label, active], i) => {
    const px = sx + i * 150;
    out += `${circle(px, y + 126, 16, active ? C.accent : C.panel2, active ? C.accent : C.line)}
      ${text(px, y + 132, num, 13, 760, active ? '#fff' : C.faint, 'middle')}
      ${text(px + 26, y + 131, label, 13, 650, active ? C.ink : C.faint)}
      ${i < 2 ? line(px + 82, y + 126, px + 122, y + 126, active ? C.accent : C.line, 2) : ''}`;
  });
  out += `${text(x + 286, y + 184, '昵称', 13, 700, C.text)}
    ${rect(x + 286, y + 198, 424, 42, 6, '#fff', C.line)}
    ${text(x + 304, y + 225, 'Lily-PC', 14, 500, C.ink)}
    ${text(x + 286, y + 270, '网络发现', 13, 700, C.text)}
    ${card(x + 286, y + 284, 424, 66)}
    ${icon('wifi', x + 306, y + 305, 24, C.accent)}
    ${text(x + 342, y + 312, '启用局域网设备自动发现', 14, 700, C.ink)}
    ${text(x + 342, y + 334, '推荐开启，可在设置中关闭。', 12, 400, C.muted)}
    ${toggle(x + 646, y + 305, true)}
    ${text(x + 286, y + 382, '默认下载目录', 13, 700, C.text)}
    ${rect(x + 286, y + 396, 330, 42, 6, '#fff', C.line)}
    ${text(x + 304, y + 423, 'C:/Users/Lily/Downloads/FengSui', 13, 400, C.muted)}
    ${button(x + 626, y + 396, 84, 42, '浏览', 'ghost', 'folder')}
    ${rect(x + 286, y + 468, 424, 56, 8, C.greenSoft, '#bee6d3')}
    ${icon('check', x + 306, y + 485, 22, C.green)}
    ${text(x + 338, y + 492, '准备完成', 14, 760, '#147b57')}
    ${text(x + 338, y + 513, '点击完成后进入主窗口。', 12, 400, C.muted)}
    ${button(x + 510, y + 552, 92, 38, '上一步', 'ghost')}
    ${button(x + 618, y + 552, 92, 38, '完成', 'primary')}
  `;
  return svgWrap(out);
}

function drawMainMessages(s) {
  const content = messagesLayout(false);
  return svgWrap(canvasHeader(s) + appShell('messages', content));
}

function messagesLayout(fileMode = false) {
  const x = 42 + 260, y = 104 + 54, w = 1196 - 260, h = 674 - 54;
  const listW = 280;
  let out = `${rect(x, y, w, h, 0, '#f6f8f9')}
    ${rect(x, y, listW, h, 0, '#fff', C.line)}
    ${line(x + listW, y, x + listW, y + h, C.line)}
    ${sectionTitle(x + 24, y + 46, '消息', '最近会话按最后消息时间排序')}
    ${rect(x + 24, y + 82, listW - 48, 34, 6, C.panel2, C.line)}
    ${icon('search', x + 36, y + 90, 17, C.faint)}
    ${text(x + 62, y + 104, '搜索会话', 12, 400, C.faint)}
  `;
  const convs = [
    ['设', '设计组', '张三：我看到你共享的目录了', '10:23', '3', true],
    ['财', '财务机-01', '报价模板已下载完成', '09:45', '', false],
    ['测', '测试机-Mac', '收到 build.zip', '昨天', '', false],
    ['会', '会议室屏幕', '在线，可发送文件', '周二', '', false],
  ];
  convs.forEach((c, i) => {
    const yy = y + 134 + i * 74;
    out += rect(x + 12, yy, listW - 24, 64, 6, c[5] ? C.accentSoft : '#fff', c[5] ? '#b8deda' : 'none');
    out += avatar(x + 42, yy + 32, c[0], c[5] ? '#fff' : C.blueSoft, c[5] ? C.accent : C.blue);
    out += text(x + 74, yy + 25, c[1], 14, 760, C.ink);
    out += text(x + 74, yy + 47, c[2], 12, 400, C.muted);
    out += text(x + listW - 54, yy + 25, c[3], 11, 400, C.faint, 'end');
    if (c[4]) out += pill(x + listW - 46, yy + 36, c[4], { width: 24, height: 20, size: 10, fill: C.red, stroke: C.red, color: '#fff' });
  });
  const cx = x + listW, cw = w - listW;
  out += `${rect(cx, y, cw, h, 0, '#fbfcfd')}
    ${rect(cx, y, cw, 64, 0, '#fff', C.line)}
    ${text(cx + 28, y + 39, '设计组', 18, 760, C.ink)}
    ${statusDot(cx + 122, y + 33, C.green)}
    ${text(cx + 138, y + 38, '12 人在线', 12, 650, C.green)}
    ${button(cx + cw - 136, y + 16, 104, 32, '浏览共享', 'soft', 'folder')}
  `;
  const bubbles = [
    ['left', 96, 240, '你把项目资料共享出来了吗？'],
    ['right', 164, 330, '已经开了 /ProjectShare，你直接到共享文件里下载。'],
    ['left', 238, 220, '看到了，速度挺快。'],
  ];
  bubbles.forEach(([side, by, bw, msg]) => {
    const isRight = side === 'right';
    const bx = isRight ? cx + cw - bw - 42 : cx + 42;
    out += `${rect(bx, y + by, bw, 46, 8, isRight ? '#d9f2ec' : '#fff', isRight ? '#b8e4d6' : C.line)}
      ${text(bx + 16, y + by + 29, msg, 13, 400, C.text)}`;
  });
  if (fileMode) {
    out += `${rect(cx + 42, y + 312, cw - 84, 88, 8, C.amberSoft, '#f0d99e')}
      ${icon('file', cx + 62, y + 334, 30, C.amber)}
      ${text(cx + 106, y + 343, 'build_20260605.zip', 14, 760, C.ink)}
      ${text(cx + 106, y + 366, '1.36 GB · 等待对方接收', 12, 400, C.muted)}
      ${button(cx + cw - 170, y + 336, 92, 34, '取消', 'ghost', 'x')}
      ${rect(cx + 42, y + 420, cw - 84, 104, 8, '#fff', C.line)}
      ${rect(cx + 58, y + 438, cw - 116, 68, 8, '#f7fbfa', '#b8deda', 1, 'stroke-dasharray="5 5"')}
      ${icon('transfer', cx + 82, y + 456, 28, C.accent)}
      ${text(cx + 126, y + 466, '拖拽文件或文件夹到此区域发送', 14, 720, C.ink)}
      ${text(cx + 126, y + 490, '发送前接收方可选择接受或拒绝。', 12, 400, C.muted)}
    `;
  }
  out += `${rect(cx + 28, y + h - 94, cw - 56, 68, 8, '#fff', C.line)}
    ${text(cx + 48, y + h - 56, fileMode ? '输入消息，或继续拖入更多文件...' : '输入消息，或拖拽文件/文件夹到这里发送', 13, 400, C.faint)}
    ${button(cx + cw - 126, y + h - 72, 80, 38, '发送', 'primary', 'send')}
  `;
  return out;
}

function drawContacts(s) {
  const x = 42 + 260, y = 104 + 54, w = 1196 - 260, h = 674 - 54;
  let out = `${rect(x, y, w, h, 0, '#fbfcfd')}
    ${sectionTitle(x + 28, y + 46, '联系人', '自动发现的在线设备，也支持手动 IP 添加。')}
    ${button(x + w - 166, y + 22, 132, 36, '添加设备', 'primary', 'plus')}
    ${rect(x + 28, y + 82, 536, 36, 6, C.panel2, C.line)}
    ${icon('search', x + 42, y + 91, 18, C.faint)}
    ${text(x + 70, y + 105, '搜索昵称、设备名或 IP', 12, 400, C.faint)}
    ${pill(x + 584, y + 86, '在线 12', { fill: C.greenSoft, stroke: '#bee6d3', color: C.green, width: 74 })}
    ${pill(x + 668, y + 86, '离线 2', { fill: C.panel2, stroke: C.line, color: C.muted, width: 74 })}
  `;
  const peers = [
    ['张', '张三-PC', 'Windows 11', '192.168.10.31:8787', 'Win', C.blueSoft, C.blue],
    ['李', '设计-Mac', 'macOS 15', '192.168.10.42:8787', 'macOS', C.greenSoft, C.green],
    ['测', '测试机-Ubuntu', 'Ubuntu 24.04', '192.168.10.53:8787', 'Linux', C.amberSoft, C.amber],
    ['财', '财务机-01', 'Windows 10', '192.168.10.77:8787', 'Win', C.blueSoft, C.blue],
  ];
  peers.forEach((p, i) => {
    const yy = y + 144 + i * 82;
    out += rowBg(x + 28, yy, 560, 68, i);
    out += avatar(x + 64, yy + 34, p[0], p[5], p[6]);
    out += text(x + 100, yy + 26, p[1], 15, 760, C.ink);
    out += text(x + 100, yy + 49, `${p[2]} · ${p[3]}`, 12, 400, C.muted);
    out += pill(x + 454, yy + 20, p[4], { width: 62, height: 26, size: 11, fill: C.panel2, stroke: C.line, color: C.muted });
    out += statusDot(x + 538, yy + 34, C.green) + text(x + 554, yy + 39, '在线', 12, 650, C.green);
  });
  out += `${card(x + 626, y + 144, 280, 276)}
    ${text(x + 652, y + 184, '手动添加 IP', 18, 760, C.ink)}
    ${text(x + 652, y + 210, 'UDP 发现不可用时可直接连接。', 12, 400, C.muted)}
    ${text(x + 652, y + 252, 'IP 地址', 12, 700, C.text)}
    ${rect(x + 652, y + 266, 228, 38, 6, '#fff', C.line)}
    ${text(x + 668, y + 290, '192.168.10.88', 13, 500, C.ink)}
    ${text(x + 652, y + 330, '端口', 12, 700, C.text)}
    ${rect(x + 652, y + 344, 100, 38, 6, '#fff', C.line)}
    ${text(x + 668, y + 368, '8787', 13, 500, C.ink)}
    ${button(x + 764, y + 344, 116, 38, '测试连接', 'ghost', 'wifi')}
    ${button(x + 652, y + 458 - 52, 228, 38, '添加到联系人', 'primary', 'plus')}
    ${card(x + 626, y + 446, 280, 118, 8, C.greenSoft)}
    ${icon('check', x + 652, y + 470, 24, C.green)}
    ${text(x + 688, y + 476, '连接成功', 14, 760, '#147b57')}
    ${text(x + 688, y + 500, '对方将出现在在线设备列表。', 12, 400, C.muted)}
  `;
  return svgWrap(canvasHeader(s) + appShell('contacts', out));
}

function drawChatFile(s) {
  return svgWrap(canvasHeader(s) + appShell('messages', messagesLayout(true)));
}

function drawBroadcast(s) {
  const x = 42 + 260, y = 104 + 54, w = 1196 - 260, h = 674 - 54;
  let out = `${rect(x, y, w, h, 0, '#fbfcfd')}
    ${sectionTitle(x + 28, y + 46, '广播通知', '向所有在线设备发送通知，可要求确认收到。')}
    ${card(x + 28, y + 94, 520, 450)}
    ${text(x + 56, y + 136, '通知内容', 16, 760, C.ink)}
    ${text(x + 56, y + 174, '标题', 12, 700, C.text)}
    ${rect(x + 56, y + 188, 460, 38, 6, '#fff', C.line)}
    ${text(x + 72, y + 212, '今日 18:00 前提交测试记录', 13, 500, C.ink)}
    ${text(x + 56, y + 258, '正文', 12, 700, C.text)}
    ${rect(x + 56, y + 272, 460, 130, 6, '#fff', C.line)}
    ${multiLine(x + 72, y + 300, ['请各位完成今天的联调记录，并把日志文件', '投递到“0605 联调日志收集”投递箱。'], 13, 24, C.ink)}
    ${rect(x + 56, y + 426, 460, 56, 8, C.panel2, C.line)}
    ${rect(x + 74, y + 444, 18, 18, 4, C.accent, C.accent)}
    ${icon('check', x + 75, y + 445, 16, '#fff')}
    ${text(x + 106, y + 458, '需要确认收到', 14, 700, C.ink)}
    ${text(x + 106, y + 478, '接收方弹窗包含“已收到”按钮。', 12, 400, C.muted)}
    ${button(x + 384, y + 500, 132, 38, '发送广播', 'primary', 'send')}
    ${card(x + 584, y + 94, 294, 450)}
    ${text(x + 612, y + 136, '确认统计', 16, 760, C.ink)}
    ${pill(x + 612, y + 158, '5 / 12 已确认', { width: 112, fill: C.greenSoft, stroke: '#bee6d3', color: C.green })}
    ${progress(x + 612, y + 202, 220, 0.42, C.green)}
  `;
  const acks = [
    ['张三-PC', '已确认', C.green, '10:25'],
    ['设计-Mac', '已确认', C.green, '10:26'],
    ['测试机-Ubuntu', '未确认', C.amber, '-'],
    ['财务机-01', '未确认', C.amber, '-'],
  ];
  acks.forEach((a, i) => {
    const yy = y + 238 + i * 62;
    out += rowBg(x + 612, yy, 220, 50, i);
    out += text(x + 630, yy + 22, a[0], 13, 700, C.ink);
    out += text(x + 630, yy + 40, a[3], 11, 400, C.faint);
    out += text(x + 812, yy + 31, a[1], 12, 700, a[2], 'end');
  });
  return svgWrap(canvasHeader(s) + appShell('messages', out));
}

function drawTransferCenter(s) {
  const x = 42 + 260, y = 104 + 54, w = 1196 - 260, h = 674 - 54;
  let out = `${rect(x, y, w, h, 0, '#fbfcfd')}
    ${sectionTitle(x + 28, y + 46, '传输中心', '所有发送、接收、共享下载任务的统一视图。')}
    ${pill(x + 650, y + 24, '全部', { width: 64, fill: C.sidebar, stroke: C.sidebar, color: '#fff' })}
    ${pill(x + 724, y + 24, '进行中', { width: 72 })}
    ${pill(x + 806, y + 24, '已完成', { width: 72 })}
    ${pill(x + 888, y + 24, '失败', { width: 58 })}
    ${rect(x + 28, y + 86, 890, 38, 6, C.panel2, C.line)}
    ${icon('search', x + 42, y + 95, 18, C.faint)}
    ${text(x + 70, y + 109, '搜索文件名、设备或失败原因', 12, 400, C.faint)}
    ${tableHeader(x + 28, y + 146, 890, [['方向', 24], ['文件名', 104], ['对端', 344], ['状态', 498], ['进度', 610], ['速度', 746], ['操作', 826]])}
  `;
  const rows = [
    ['发送', '架构说明.pdf', '张三-PC', '进行中', '67%', '18 MB/s', '暂停', C.green, .67],
    ['接收', 'logo.png', '测试机-Mac', '已完成', '100%', '-', '打开', C.green, 1],
    ['发送', 'build.zip', '财务机-01', '失败', '12%', '0', '重试', C.red, .12],
    ['下载', '课件.zip', '老师-PC', '排队中', '0%', '-', '取消', C.amber, .02],
  ];
  rows.forEach((r, i) => {
    const yy = y + 194 + i * 64;
    out += rowBg(x + 28, yy, 890, 54, i);
    out += text(x + 52, yy + 33, r[0], 13, 650, C.text);
    out += icon('file', x + 132, yy + 16, 20, C.muted);
    out += text(x + 160, yy + 33, r[1], 13, 700, C.ink);
    out += text(x + 372, yy + 33, r[2], 13, 500, C.text);
    out += text(x + 526, yy + 33, r[3], 13, 760, r[7]);
    out += text(x + 638, yy + 25, r[4], 12, 500, C.text);
    out += progress(x + 638, yy + 34, 78, r[8], r[7] === C.red ? C.red : C.green);
    out += text(x + 774, yy + 33, r[5], 12, 500, C.text);
    out += text(x + 854, yy + 33, r[6], 12, 760, r[7], 'middle');
  });
  out += `${card(x + 28, y + 480, 890, 92, 8, C.redSoft)}
    ${icon('x', x + 54, y + 510, 24, C.red)}
    ${text(x + 92, y + 518, 'build.zip 传输失败', 15, 760, C.red)}
    ${text(x + 92, y + 544, '原因：连接中断 / 对方离线。建议：等待对方上线后重试。', 13, 400, C.muted)}
    ${button(x + 790, y + 506, 92, 36, '重试', 'danger', 'transfer')}
  `;
  return svgWrap(canvasHeader(s) + appShell('transfer', out));
}

function drawOnlineShares(s) {
  const x = 42 + 260, y = 104 + 54, w = 1196 - 260, h = 674 - 54;
  let out = `${rect(x, y, w, h, 0, '#fbfcfd')}
    ${sectionTitle(x + 28, y + 46, '共享文件', '浏览同网段在线设备开放的只读共享目录。')}
    ${pill(x + 682, y + 24, '浏览共享', { width: 88, fill: C.sidebar, stroke: C.sidebar, color: '#fff' })}
    ${pill(x + 780, y + 24, '我的共享', { width: 88 })}
    ${card(x + 28, y + 92, 248, 486)}
    ${text(x + 52, y + 130, '共享源', 15, 760, C.ink)}
  `;
  const sources = [
    ['张', '张三-PC', 'ProjectShare', true],
    ['设', '设计-Mac', 'DesignAssets', false],
    ['老', '老师-PC', 'Courseware', false],
  ];
  sources.forEach((p, i) => {
    const yy = y + 154 + i * 76;
    out += rect(x + 44, yy, 216, 62, 6, p[3] ? C.accentSoft : '#fff', p[3] ? '#b8deda' : C.softLine);
    out += avatar(x + 74, yy + 31, p[0], p[3] ? '#fff' : C.blueSoft, p[3] ? C.accent : C.blue);
    out += text(x + 108, yy + 25, p[1], 13, 760, C.ink);
    out += text(x + 108, yy + 46, p[2], 12, 400, C.muted);
  });
  out += `${card(x + 304, y + 92, 614, 486)}
    ${text(x + 332, y + 130, '张三-PC / ProjectShare', 18, 760, C.ink)}
    ${pill(x + 332, y + 148, '已授权', { width: 70, fill: C.greenSoft, stroke: '#bee6d3', color: C.green })}
    ${text(x + 332, y + 198, '路径：/ 原型资料 / 0605 评审', 13, 650, C.muted)}
    ${tableHeader(x + 332, y + 220, 548, [['名称', 24], ['大小', 284], ['修改时间', 372], ['操作', 488]])}
  `;
  const files = [
    ['folder', '设计稿', '-', '今天 09:40', '进入'],
    ['file', 'FengSui_PRD.pdf', '2.4 MB', '今天 09:18', '下载'],
    ['file', '演示脚本.docx', '860 KB', '昨天', '下载'],
    ['file', '安装包清单.xlsx', '148 KB', '昨天', '下载'],
  ];
  files.forEach((f, i) => {
    const yy = y + 268 + i * 58;
    out += rowBg(x + 332, yy, 548, 48, i);
    out += icon(f[0] === 'folder' ? 'folder' : 'file', x + 350, yy + 14, 20, f[0] === 'folder' ? C.amber : C.muted);
    out += text(x + 380, yy + 30, f[1], 13, 700, C.ink);
    out += text(x + 616, yy + 30, f[2], 12, 400, C.muted);
    out += text(x + 704, yy + 30, f[3], 12, 400, C.muted);
    out += text(x + 830, yy + 30, f[4], 12, 760, C.accent, 'middle');
  });
  return svgWrap(canvasHeader(s) + appShell('share', out));
}

function drawMyShares(s) {
  const x = 42 + 260, y = 104 + 54, w = 1196 - 260, h = 674 - 54;
  let out = `${rect(x, y, w, h, 0, '#fbfcfd')}
    ${sectionTitle(x + 28, y + 46, '我的共享', '管理本机目录的在线可见状态。')}
    ${pill(x + 682, y + 24, '浏览共享', { width: 88 })}
    ${pill(x + 780, y + 24, '我的共享', { width: 88, fill: C.sidebar, stroke: C.sidebar, color: '#fff' })}
    ${button(x + w - 168, y + 22, 132, 36, '添加共享', 'primary', 'plus')}
  `;
  const shares = [
    ['ProjectShare', 'D:/Work/ProjectShare', '18 个文件 · 3.2 GB', true],
    ['安装包', 'D:/Release/FengSui', '4 个文件 · 682 MB', true],
    ['临时资料', 'D:/Temp/Review', '7 个文件 · 140 MB', false],
  ];
  shares.forEach((sh, i) => {
    const yy = y + 106 + i * 116;
    out += card(x + 28, yy, 890, 92);
    out += icon('folder', x + 56, yy + 28, 32, sh[3] ? C.accent : C.faint);
    out += text(x + 106, yy + 34, sh[0], 16, 760, C.ink);
    out += text(x + 106, yy + 58, sh[1], 12, 400, C.muted);
    out += text(x + 106, yy + 78, sh[2], 12, 400, C.faint);
    out += pill(x + 636, yy + 32, sh[3] ? '网络可见' : '已停用', { width: 84, fill: sh[3] ? C.greenSoft : C.panel2, stroke: sh[3] ? '#bee6d3' : C.line, color: sh[3] ? C.green : C.muted });
    out += toggle(x + 744, yy + 34, sh[3]);
    out += button(x + 814, yy + 30, 76, 34, '删除', 'ghost', 'trash');
  });
  out += `${card(x + 28, y + 478, 890, 84, 8, C.accentSoft)}
    ${icon('shield', x + 58, y + 504, 28, C.accent)}
    ${text(x + 102, y + 510, '首次访问授权', 15, 760, C.ink)}
    ${text(x + 102, y + 535, '未授权设备首次浏览共享目录时，本机将弹出确认窗口。', 13, 400, C.muted)}
  `;
  return svgWrap(canvasHeader(s) + appShell('share', out));
}

function drawPermissionModal(s) {
  const base = appShell('share', `${rect(302, 158, 936, 620, 0, 'rgba(31,41,51,.36)')}`);
  const mx = 430, my = 234, mw = 520, mh = 318;
  let out = `${base}
    ${rect(mx, my, mw, mh, 10, '#fff', C.line, 1, 'filter="url(#softShadow)"')}
    ${icon('shield', mx + 34, my + 34, 34, C.accent)}
    ${text(mx + 84, my + 52, '共享访问请求', 22, 780, C.ink)}
    ${text(mx + 84, my + 78, '对方正在请求浏览你的共享目录。', 13, 400, C.muted)}
    ${card(mx + 34, my + 112, mw - 68, 86, 8, C.panel2)}
    ${avatar(mx + 70, my + 155, '张', C.blueSoft, C.blue)}
    ${text(mx + 110, my + 144, '张三-PC', 15, 760, C.ink)}
    ${text(mx + 110, my + 168, 'Windows 11 · 192.168.10.31:8787', 12, 400, C.muted)}
    ${text(mx + 34, my + 232, '请求访问：ProjectShare / 0605 评审', 13, 700, C.text)}
    ${rect(mx + 34, my + 254, 18, 18, 4, '#fff', C.line)}
    ${text(mx + 64, my + 268, '记住我的选择，同设备后续访问不再询问', 13, 400, C.muted)}
    ${button(mx + 248, my + 290, 92, 38, '拒绝', 'ghost', 'x')}
    ${button(mx + 356, my + 290, 112, 38, '允许访问', 'primary', 'check')}
  `;
  return svgWrap(canvasHeader(s) + out);
}

function drawDropboxList(s) {
  const x = 42 + 260, y = 104 + 54, w = 1196 - 260, h = 674 - 54;
  let out = `${rect(x, y, w, h, 0, '#fbfcfd')}
    ${sectionTitle(x + 28, y + 46, '文件投递箱', '创建资料收集任务，按提交人自动归档。')}
    ${button(x + w - 168, y + 22, 132, 36, '新建投递', 'primary', 'plus')}
  `;
  const tasks = [
    ['0605 联调日志收集', '截止 今天 18:00', '5 / 12 已提交', .42, C.green],
    ['办公室照片归档', '截止 明天 12:00', '9 / 10 已提交', .9, C.green],
    ['PRD 评审反馈', '已结束', '12 / 12 已提交', 1, C.muted],
  ];
  tasks.forEach((t, i) => {
    const yy = y + 102 + i * 126;
    out += card(x + 28, yy, 890, 100);
    out += icon('inbox', x + 58, yy + 30, 34, i === 2 ? C.faint : C.accent);
    out += text(x + 110, yy + 34, t[0], 17, 760, C.ink);
    out += text(x + 110, yy + 60, t[1], 12, 400, C.muted);
    out += text(x + 110, yy + 84, t[2], 12, 700, t[4]);
    out += progress(x + 642, yy + 44, 180, t[3], t[4]);
    out += button(x + 834, yy + 34, 58, 34, '查看', 'ghost');
  });
  out += `${card(x + 28, y + 500, 890, 68, 8, C.purpleSoft)}
    ${text(x + 58, y + 532, 'P1 能力', 14, 760, C.purple)}
    ${text(x + 130, y + 532, '投递箱不进入 P0 主导航源码，但高保真原型保留产品目标态。', 13, 400, C.muted)}
  `;
  return svgWrap(canvasHeader(s) + appShell('dropbox', out));
}

function drawCreateDropbox(s) {
  const x = 42 + 260, y = 104 + 54, w = 1196 - 260, h = 674 - 54;
  let out = `${rect(x, y, w, h, 0, '#fbfcfd')}
    ${sectionTitle(x + 28, y + 46, '新建投递任务', '填写任务名、说明、截止时间并创建。')}
    ${card(x + 28, y + 94, 594, 474)}
    ${text(x + 58, y + 138, '任务名', 12, 700, C.text)}
    ${rect(x + 58, y + 152, 520, 40, 6, '#fff', C.line)}
    ${text(x + 74, y + 178, '0605 联调日志收集', 13, 500, C.ink)}
    ${text(x + 58, y + 224, '描述', 12, 700, C.text)}
    ${rect(x + 58, y + 238, 520, 112, 6, '#fff', C.line)}
    ${multiLine(x + 74, y + 266, ['请提交本机联调日志、截图和简要说明。', '文件会按提交人自动归档。'], 13, 24, C.ink)}
    ${text(x + 58, y + 382, '截止时间', 12, 700, C.text)}
    ${rect(x + 58, y + 396, 246, 40, 6, '#fff', C.line)}
    ${text(x + 74, y + 422, '2026-06-05 18:00', 13, 500, C.ink)}
    ${text(x + 332, y + 382, '可见范围', 12, 700, C.text)}
    ${rect(x + 332, y + 396, 246, 40, 6, '#fff', C.line)}
    ${text(x + 348, y + 422, '所有在线设备', 13, 500, C.ink)}
    ${button(x + 444, y + 488, 134, 40, '创建任务', 'primary', 'check')}
    ${card(x + 650, y + 94, 268, 276, 8, C.panel2)}
    ${text(x + 678, y + 134, '创建后效果', 16, 760, C.ink)}
    ${icon('inbox', x + 680, y + 168, 32, C.accent)}
    ${text(x + 724, y + 186, '同网段设备可见', 14, 700, C.ink)}
    ${text(x + 724, y + 210, '点击进入即可提交文件。', 12, 400, C.muted)}
    ${line(x + 680, y + 244, x + 888, y + 244, C.line)}
    ${text(x + 680, y + 282, '创建者可查看提交人、文件名、提交时间，并标记“已收到”。', 12, 400, C.muted)}
  `;
  return svgWrap(canvasHeader(s) + appShell('dropbox', out));
}

function drawSettings(s) {
  const x = 42 + 260, y = 104 + 54, w = 1196 - 260, h = 674 - 54;
  let out = `${rect(x, y, w, h, 0, '#fbfcfd')}
    ${sectionTitle(x + 28, y + 46, '设置', '常规设置与网络发现配置即时写入本地数据库。')}
    ${pill(x + 724, y + 24, '常规', { width: 64 })}
    ${pill(x + 798, y + 24, '网络', { width: 64, fill: C.sidebar, stroke: C.sidebar, color: '#fff' })}
    ${button(x + 872, y + 22, 62, 36, '诊断', 'ghost', 'wifi')}
    ${card(x + 28, y + 92, 420, 462)}
    ${text(x + 58, y + 132, '常规', 17, 760, C.ink)}
    ${text(x + 58, y + 174, '昵称', 12, 700, C.text)}
    ${rect(x + 58, y + 188, 330, 38, 6, '#fff', C.line)}
    ${text(x + 74, y + 212, 'Lily-PC', 13, 500, C.ink)}
    ${text(x + 58, y + 258, '默认下载目录', 12, 700, C.text)}
    ${rect(x + 58, y + 272, 248, 38, 6, '#fff', C.line)}
    ${text(x + 74, y + 296, 'D:/Downloads/FengSui', 12, 400, C.muted)}
    ${button(x + 318, y + 272, 70, 38, '浏览', 'ghost')}
    ${text(x + 58, y + 354, '开机启动', 13, 650, C.text)}${toggle(x + 342, y + 338, false)}
    ${text(x + 58, y + 404, '最小化到托盘', 13, 650, C.text)}${toggle(x + 342, y + 388, true)}
    ${card(x + 478, y + 92, 440, 462)}
    ${text(x + 508, y + 132, '网络', 17, 760, C.ink)}
    ${text(x + 508, y + 174, '启用局域网设备自动发现', 13, 650, C.text)}${toggle(x + 822, y + 158, true)}
    ${text(x + 508, y + 226, '监听端口', 12, 700, C.text)}
    ${rect(x + 508, y + 240, 130, 38, 6, '#fff', C.line)}
    ${text(x + 524, y + 264, '8787', 13, 500, C.ink)}
    ${pill(x + 652, y + 246, '修改后需重启网络服务', { width: 156, fill: C.amberSoft, stroke: '#f3d38a', color: C.amber, size: 11 })}
    ${text(x + 508, y + 320, '手动添加 IP', 12, 700, C.text)}
    ${rect(x + 508, y + 334, 180, 38, 6, '#fff', C.line)}
    ${text(x + 524, y + 358, '192.168.10.88', 13, 400, C.muted)}
    ${rect(x + 700, y + 334, 76, 38, 6, '#fff', C.line)}
    ${text(x + 716, y + 358, '8787', 13, 400, C.muted)}
    ${button(x + 790, y + 334, 88, 38, '添加', 'primary', 'plus')}
  `;
  return svgWrap(canvasHeader(s) + appShell('settings', out));
}

function drawDiagnostics(s) {
  const x = 42 + 260, y = 104 + 54, w = 1196 - 260, h = 674 - 54;
  let out = `${rect(x, y, w, h, 0, '#fbfcfd')}
    ${sectionTitle(x + 28, y + 46, '网络诊断', '自动检测本机网络、监听端口和广播连通性。')}
    ${button(x + w - 148, y + 22, 112, 36, '重新检测', 'primary', 'wifi')}
  `;
  const checks = [
    ['本机 IP', '192.168.10.24 / 10.8.0.2', '正常', C.green, 'check'],
    ['监听端口', 'TCP 8787 已监听，UDP 8788 已绑定', '正常', C.green, 'check'],
    ['防火墙', 'Windows Defender 已允许 FengSui', '正常', C.green, 'check'],
    ['UDP 广播测试', '已收到本机回包，局域网广播可用', '通过', C.green, 'check'],
    ['在线设备数', '当前发现 12 台在线设备', '12', C.accent, 'users'],
    ['诊断日志', '可导出最近发现与传输摘要', '可导出', C.amber, 'file'],
  ];
  checks.forEach((c, i) => {
    const col = i % 2;
    const row = Math.floor(i / 2);
    const bx = x + 28 + col * 456;
    const by = y + 102 + row * 138;
    out += card(bx, by, 420, 106);
    out += icon(c[4], bx + 26, by + 34, 30, c[3]);
    out += text(bx + 74, by + 36, c[0], 15, 760, C.ink);
    out += text(bx + 74, by + 62, c[1], 12, 400, C.muted);
    out += pill(bx + 314, by + 36, c[2], { width: 78, fill: c[3] === C.green ? C.greenSoft : C.amberSoft, stroke: c[3] === C.green ? '#bee6d3' : '#f3d38a', color: c[3], size: 11 });
  });
  out += `${card(x + 28, y + 532, 876, 48, 8, C.panel2)}
    ${text(x + 52, y + 562, '建议：设备不可见时，先确认双方位于同网段，再尝试联系人页的手动 IP 添加。', 13, 500, C.muted)}
  `;
  return svgWrap(canvasHeader(s) + appShell('settings', out));
}

function drawTray(s) {
  let out = canvasHeader(s);
  out += `${rect(112, 132, 1056, 560, 12, '#f7f9fa', C.line, 1, 'filter="url(#softShadow)"')}
    ${text(154, 184, '桌面托盘场景', 24, 760, C.ink)}
    ${text(154, 212, '主窗口关闭后，烽燧仍可在托盘中保持在线状态。', 13, 400, C.muted)}
    ${rect(154, 272, 620, 310, 10, '#fff', C.line)}
    ${text(190, 320, '烽燧 FengSui', 20, 760, C.ink)}
    ${statusDot(194, 356, C.green)}${text(212, 361, '当前在线 · 12 台设备', 13, 650, C.green)}
    ${multiLine(190, 410, ['新消息、传输完成和共享授权请求会通过系统通知进入。', '托盘是增强入口，主窗口仍是完整功能入口。'], 13, 24, C.muted)}
    ${rect(858, 292, 232, 222, 8, '#fff', C.line, 1, 'filter="url(#tinyShadow)"')}
    ${text(884, 330, '烽燧 FengSui', 14, 760, C.ink)}
    ${line(878, 350, 1070, 350, C.line)}
    ${text(884, 382, '切换为隐身', 13, 500, C.text)}
    ${text(884, 424, '打开主窗口', 13, 500, C.text)}
    ${text(884, 466, '退出', 13, 500, C.red)}
    ${circle(1052, 610, 18, C.sidebar)}
    ${icon('wifi', 1042, 600, 20, '#fff')}
    ${svgPath('M 1052 590 L 1010 514', C.faint, 1.5, 'none', 'stroke-dasharray="4 5"')}
  `;
  return svgWrap(out);
}

function drawInformationArchitecture(s) {
  let out = canvasHeader(s);
  out += `${rect(76, 120, 1128, 616, 12, '#fff', C.line, 1, 'filter="url(#softShadow)"')}
    ${text(114, 172, '信息架构与核心链路', 25, 780, C.ink)}
    ${text(114, 200, 'P0 聚焦可靠收发与共享闭环，P1 扩展广播、投递箱与托盘体验。', 13, 400, C.muted)}
  `;
  const nodes = [
    ['首次启动', 120, 282, C.greenSoft, C.green],
    ['主窗口', 300, 282, C.accentSoft, C.accent],
    ['消息/会话', 500, 214, C.blueSoft, C.blue],
    ['联系人', 500, 350, C.blueSoft, C.blue],
    ['文件传输', 700, 214, C.greenSoft, C.green],
    ['共享文件', 700, 350, C.greenSoft, C.green],
    ['投递箱', 900, 214, C.purpleSoft, C.purple],
    ['设置/诊断', 900, 350, C.amberSoft, C.amber],
  ];
  nodes.forEach((n) => {
    out += rect(n[1], n[2], 134, 58, 8, n[3], n[4]);
    out += text(n[1] + 67, n[2] + 36, n[0], 15, 760, n[4], 'middle');
  });
  const arrows = [
    [254, 311, 300, 311],
    [434, 298, 500, 243],
    [434, 324, 500, 379],
    [634, 243, 700, 243],
    [634, 379, 700, 379],
    [834, 243, 900, 243],
    [834, 379, 900, 379],
  ];
  arrows.forEach((a) => out += line(...a, C.faint, 2, 'marker-end="url(#arrow)"'));
  out += `${rect(114, 510, 1010, 126, 8, C.panel2, C.line)}
    ${text(144, 552, 'P0 主链路：首次启动 → 自动发现 / 手动 IP → 联系人 → 单聊消息 / 文件拖拽 → 传输中心记录', 14, 720, C.ink)}
    ${text(144, 588, '共享链路：添加共享目录 → 他人请求浏览 → 首次授权 → 目录浏览 / 下载 → 本地记录', 14, 720, C.ink)}
    ${text(144, 624, 'P1 扩展：广播确认、投递箱、托盘增强；不作为当前源码已完成能力误导展示。', 14, 720, C.ink)}
  `;
  return svgWrap(out);
}

async function main() {
  fs.mkdirSync(outDir, { recursive: true });
  for (const screen of screens) {
    const svg = screen.draw(screen);
    const svgPath = path.join(outDir, screen.file);
    const pngPath = svgPath.replace(/\.svg$/i, '.png');
    fs.writeFileSync(svgPath, svg, 'utf8');
    if (sharp) {
      await sharp(Buffer.from(svg)).png().toFile(pngPath);
    }
  }
  const manifest = screens.map((screen) => ({
    file: screen.file,
    png: screen.file.replace(/\.svg$/i, '.png'),
    title: screen.title,
    subtitle: screen.subtitle,
    priority: screen.priority,
    source: screen.source,
  }));
  fs.writeFileSync(path.join(__dirname, 'screens', 'manifest.json'), JSON.stringify(manifest, null, 2), 'utf8');
  console.log(`Generated ${screens.length} SVG${sharp ? ' and PNG' : ''} prototype screens.`);
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});

