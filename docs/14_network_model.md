# 14 — 长期网络模型与安全边界

> Status: Accepted
> Scope: Long-term network model
> Implementation: Not started
> Code impact: None in this change

本文定义 FengSui 的长期网络模型、安全边界、自动发现策略、手动连接策略和 UI 配置基线。它是后续网络能力改造的设计依据，不是当前代码行为说明。

FengSui 的默认定位是内网文件协作工具，面向普通内网、弱外网、完全断网、高安全要求环境。ZeroTier、Tailscale、虚拟网卡和本机多实例只作为测试多网卡、虚拟网卡、异地链路的工具，不作为产品正式默认网络假设。

---

## 1. 非目标

本文不解决以下问题：

- 公网穿透
- NAT 打洞
- 中心服务器
- 中继服务
- 完整身份认证
- 端到端加密
- 自动防火墙配置

这些能力可以作为独立安全或远程协作里程碑讨论，但不能混入 FengSui 的默认内网网络模型。默认模型必须先保证最小暴露、可审计、可配置、可诊断。

---

## 2. 核心原则

FengSui 的长期网络策略遵循以下原则：

- 默认只使用被授权的内网接口。
- 默认只在允许的内网网段内发现和连接设备。
- 默认不监听所有网卡。
- 默认不跨越非授权网卡或安全域。
- 默认不自动信任发现到的设备。
- 手动 IP 是跨 VLAN、禁广播、固定 IP 环境的补充路径，不是绕过安全策略的后门。
- 多网卡支持的目的不是“尽量连上”，而是“显式授权安全域”。

现有实现中“选择第一个非回环 IPv4”的行为只能作为过渡状态，长期应替换为明确的 `NetworkPolicy`。

---

## 3. 网络模式

FengSui 长期保留三种网络模式。

| 策略项 | `secure_lan` | `multi_lan` | `compat_test` |
|---|---|---|---|
| 默认模式 | 是 | 否 | 否 |
| 监听 `0.0.0.0` | 否 | 否 | 可选 |
| 绑定具体 IP | 是，单个授权 IP | 是，多个授权 IP | 可选 |
| 自动发现网卡 | 授权单网卡 | 授权多网卡 | 所有或测试网卡 |
| 虚拟网卡默认启用 | 否 | 否 | 可启用 |
| 手动 IP | 受 CIDR 限制 | 受 CIDR 限制 | 可放宽 |
| 适用场景 | 普通内网 | 多安全域内网 | 测试、兼容、ZeroTier、本机多实例 |

### 3.1 安全内网模式 `secure_lan`

`secure_lan` 是正式默认模式，适合单一受信任内网。

行为：

- 只绑定一个被授权的内网接口 IP。
- 只在该接口所在网段进行发现。
- 只宣告该接口对应的 IP。
- 手动 IP 必须落在允许 CIDR 内。
- 不启用 ZeroTier、Tailscale、VMware、Hyper-V、WSL 等虚拟网卡，除非用户显式授权。

示例：

```text
授权网卡：Ethernet
本机地址：192.168.10.25/24
监听地址：192.168.10.25:8787
发现范围：192.168.10.0/24
```

### 3.2 多网卡内网模式 `multi_lan`

`multi_lan` 适合一台机器同时接入多个受控内网，例如办公网、研发网、文件协作网、生产辅助网。

行为：

- 绑定多个被授权的具体 IP。
- 每个授权网卡独立发现。
- 每个网段只宣告对应接口 IP。
- 不跨网段转发发现包或业务流量。
- 不因为机器接入多个网络就自动扩大信任边界。

示例：

```text
授权网卡：
- Ethernet 192.168.10.25/24
- Ethernet 2 10.20.5.8/16

监听地址：
- 192.168.10.25:8787
- 10.20.5.8:8787
```

### 3.3 测试 / 兼容模式 `compat_test`

`compat_test` 适合开发、测试、ZeroTier、Tailscale、虚拟机、本机多实例和临时排障。

行为：

- 可选择监听 `0.0.0.0:8787`。
- 可允许所有可用网卡参与发现。
- 可启用 ZeroTier、Tailscale、虚拟机网卡。
- 可放宽手动 IP 的 CIDR 限制。
- UI 必须明确提示风险：该模式会扩大端口暴露面，不建议正式内网部署使用。

`compat_test` 不能成为默认模式，也不能被实现为静默 fallback。

---

## 4. 安全边界

### 4.1 地址与网卡边界

默认候选地址只包括受信任内网地址。长期策略应排除：

- loopback 地址。
- `169.254.0.0/16` 链路本地地址。
- 未连接或不可用网卡。
- 未授权虚拟网卡。
- 未授权公网地址。
- 未授权代理、隧道、容器、虚拟交换机地址。

RFC1918 内网地址也不能自动视为可信。只有被用户、管理员或策略显式授权的接口和 CIDR 才能进入发现、监听和手动连接路径。

### 4.2 自动发现不等于信任

自动发现只能产生候选设备，不代表该设备可信。

发现到的设备默认状态应为：

```text
候选设备 / 未信任设备
```

后续正式通信至少需要：

- 稳定设备 ID。
- 首次配对确认。
- 对端公钥或指纹。
- 握手签名。
- 信任设备列表。
- 密钥不匹配时拒绝连接。

完整身份认证、握手签名、加密传输属于后续安全里程碑，本文只固定网络边界，不展开密码协议。

### 4.3 防火墙边界

FengSui 不应默认自动创建宽泛防火墙规则。正式部署建议由管理员或用户显式放行，并限制来源网段。

示例：

```powershell
New-NetFirewallRule `
  -DisplayName "FengSui TCP 8787 Intranet" `
  -Direction Inbound `
  -Program "C:\path\to\fengsui.exe" `
  -Action Allow `
  -Protocol TCP `
  -LocalPort 8787 `
  -RemoteAddress 192.168.10.0/24
```

UDP 发现端口也应限制来源网段：

```powershell
New-NetFirewallRule `
  -DisplayName "FengSui UDP Discovery Intranet" `
  -Direction Inbound `
  -Program "C:\path\to\fengsui.exe" `
  -Action Allow `
  -Protocol UDP `
  -LocalPort 8788 `
  -RemoteAddress 192.168.10.0/24
```

---

## 5. TCP 监听策略

正式环境不应默认使用：

```text
0.0.0.0:8787
```

长期实现应由 `NetworkPolicy` 生成一个或多个明确的监听端点：

```text
192.168.10.25:8787
10.20.5.8:8787
```

如果没有可用授权网卡，网络服务应启动失败并给出明确诊断，而不是自动退回到所有网卡监听。

建议模型：

```cpp
struct BindEndpoint {
    QString interfaceId;
    QString interfaceName;
    QHostAddress ip;
    quint16 port;
};
```

`TcpServer` 长期应支持多个绑定端点。每个端点独立监听，任何一个端点启动失败都应记录诊断；只有全部失败时整体网络服务才失败。

---

## 6. 自动发现策略

自动发现必须受网络模式和授权接口约束。

### 6.1 发送范围

`secure_lan`：

- 只在授权单网卡发送发现包。
- 优先使用该网卡的 directed broadcast。
- 只宣告该网卡 IP。

`multi_lan`：

- 在每个授权网卡分别发送发现包。
- 每个发现包只宣告当前网卡对应 IP。
- 不跨网段转发。

`compat_test`：

- 可允许所有可用网卡参与发现。
- 可使用组播、广播或测试网卡。
- UI 必须展示风险提示。

### 6.2 接收校验

收到发现包后，必须校验：

- 来源地址是否在允许 CIDR 内。
- 报文宣告地址是否在允许 CIDR 内。
- 报文宣告端口是否有效。
- `peer_id` 是否为空或与本机冲突。

不满足策略的发现包应被忽略并写入诊断日志，不应进入在线设备列表。

### 6.3 协议演进

现有 `presence.hello` 使用：

```json
{
  "type": "presence.hello",
  "peer_id": "a1b2c3d4",
  "ip": "192.168.1.20",
  "tcp_port": 8787
}
```

长期可扩展为同时支持 `addresses`：

```json
{
  "type": "presence.hello",
  "peer_id": "a1b2c3d4",
  "display_name": "FengSui-PC",
  "device_name": "DESKTOP-001",
  "tcp_port": 8787,
  "addresses": [
    {
      "ip": "192.168.10.25",
      "port": 8787,
      "interface": "Ethernet",
      "scope": "secure_lan"
    }
  ],
  "version": "0.1.0"
}
```

兼容要求：

- 接收端优先使用符合策略的 `addresses`。
- 如果缺少 `addresses`，保留对旧字段 `ip` / `tcp_port` 的解析。
- 不应因为对端宣告多个地址就自动尝试所有地址。

---

## 7. 手动连接策略

手动 IP 是正式能力，不是测试能力。它用于：

- 不同 VLAN 之间路由可达但广播不可达。
- 网络禁止 UDP 广播或组播。
- 只允许固定 TCP 端口。
- 管理员分配固定内网 IP。
- 自动发现暂时不可用但业务 TCP 可达。

手动连接流程：

```text
用户输入 IP / 端口
↓
校验 IP 格式和端口范围
↓
校验目标 IP 是否属于允许 CIDR
↓
执行 TCP 探测
↓
探测成功后保存 endpoint
↓
进入候选设备或未信任设备状态
```

手动连接优先级高于自动发现结果。若用户已经成功添加 `192.168.10.33:8787`，后续自动发现不应随意覆盖该 endpoint，除非：

- 对端完成可信身份校验。
- 用户确认替换。
- 策略明确允许自动更新。

长期应保存最近成功连接地址：

```json
{
  "manual_peers": [
    {
      "name": "文件协作电脑B",
      "host": "192.168.10.33",
      "port": 8787,
      "last_success_at": "2026-06-08T10:30:00+08:00"
    }
  ]
}
```

---

## 8. UI 配置

### 8.1 首次启动向导

首次启动若检测到多个候选网卡，应让用户选择 FengSui 允许使用的内网。

示例：

```text
选择 FengSui 允许使用的网络：

[✓] Ethernet   192.168.10.25   办公网
[ ] WLAN       192.168.1.9     未启用
[ ] ZeroTier   10.101.122.163  测试网卡
[ ] VMware     192.168.56.1    虚拟网卡
[ ] Hyper-V    172.17.192.1    虚拟网卡
```

默认推荐：

1. 用户显式选择的网卡。
2. 管理员配置的允许网段。
3. 系统主物理网卡。
4. RFC1918 内网地址。
5. 虚拟网卡默认不启用。
6. `169.254.0.0/16` 禁用。
7. 公网地址禁用或强提醒。

### 8.2 设置页

设置页网络区域应包含：

- 网络模式：安全内网、多网卡内网、测试/兼容。
- 授权网卡列表。
- 允许网段列表。
- TCP 监听端口。
- UDP 发现端口。
- 当前监听地址预览。
- 当前发现范围预览。
- 手动连接历史。
- 测试/兼容模式风险提示。

示例：

```text
网络模式：安全内网模式

允许使用的网卡：
[✓] Ethernet  192.168.10.25  办公网
[ ] WLAN      192.168.1.9    未启用
[ ] ZeroTier  10.101.122.163 测试网卡

允许网段：
192.168.10.0/24
10.20.0.0/16

监听地址：
192.168.10.25:8787

发现端口：
8788
```

### 8.3 网络诊断页

诊断页应展示：

- 当前网络模式。
- 授权网卡。
- 当前监听地址。
- 当前发现接口。
- 允许 CIDR。
- 被排除网卡及原因。
- TCP 端口监听结果。
- UDP 发现端口结果。
- 防火墙建议。
- 最近一次手动连接探测结果。
- 最近一次发现报文收发结果。

诊断页必须区分：

- 当前代码未实现。
- 策略不允许。
- 防火墙阻止。
- 对端未监听。
- 对端未完成信任。

---

## 9. 配置模型

长期配置可以采用以下逻辑结构。具体持久化仍由 `settings` 表或后续配置表决定。

```json
{
  "network": {
    "mode": "secure_lan",
    "tcpPort": 8787,
    "discoveryPort": 8788,
    "discoveryEnabled": true,
    "listenMode": "selected_interfaces",
    "selectedInterfaces": [],
    "allowedCidrs": [],
    "manualPeers": [],
    "allowAllInterfacesForTesting": false
  },
  "security": {
    "requirePairing": true,
    "allowUnknownPeers": false,
    "encryptTransport": true,
    "verifyPeerKey": true
  }
}
```

默认值：

- `network.mode = "secure_lan"`
- `network.discoveryEnabled = true`
- `network.tcpPort = 8787`
- `network.discoveryPort = 8788`
- `network.allowAllInterfacesForTesting = false`
- `security.requirePairing = true`
- `security.allowUnknownPeers = false`

安全配置项是长期目标。若当前代码未实现，UI 和文档必须避免暗示已经具备对应能力。

---

## 10. 实施路线

后续任务建议按以下顺序拆分：

1. `NetworkPolicy` 纯逻辑模型。
2. 网卡枚举与分类诊断。
3. `TcpServer` 多绑定支持。
4. `UdpDiscovery` 授权接口发现。
5. `manual_peers` 与 `last_success_endpoint`。
6. 设置页网络模式与诊断页。
7. `presence.hello addresses` 扩展。
8. 配对、公钥、握手签名、加密传输。

每个任务都应附带策略单元测试，尤其是三种网络模式的行为矩阵。`secure_lan` 的默认行为必须最严格，`compat_test` 的放宽行为必须由显式设置触发。

---

## 11. 与现有文档的关系

- `04_architecture.md` 定义分层边界；本文补充网络策略和安全边界。
- `06_protocol.md` 定义当前通信协议；本文只定义长期协议演进方向，不直接修改协议。
- `08_ui_spec.md` 定义当前 UI 交互；本文补充长期网络配置和诊断 UI 的目标状态。

本文被视为长期网络模型的设计基线。后续代码实现、协议扩展和 UI 改造应以本文为默认决策来源。
