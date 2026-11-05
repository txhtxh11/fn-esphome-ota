# fn-esphome-ota

飞牛 fnOS 应用：**ESPHome 本地编译 + 局域网 OTA 推送**。

网页版 <https://esp-homekit.pages.dev> 只能云端编译、浏览器串口烧录；这个应用把「编译」和「OTA」搬到你的 NAS 上，页面上点一下就能把固件推到内网任何 ESP8266/ESP32。

## 特性

- **本地编译**：NAS 上跑真正的 `esphome compile`，产物与命令行完全一致
- **一键 OTA**：编译完成后直接 `esphome upload` 推到设备 IP，带实时日志
- **体积拦截**：超过 520 KB 的固件拒绝推送（1MB 设备写坏后只能线刷）
- **页面同源**：直接复用 `ui.html` 生成配置，右下角出现「本地 OTA」面板
- **两种模式**：线上版（pages.dev）保持不变；本地版检测不到 `/api/health` 时面板自动隐藏

## 安装

### 方式一：应用中心手动安装

1. 下载 `dist/esphome-ota-1.0.0.fpk`
2. 飞牛 OS → 应用中心 → 手动安装
3. 安装过程会在后台创建 Python 环境并安装 esphome（约 5–15 分钟，看网速）
4. 打开 `http://<NAS-IP>:8765`

### 方式二：手动部署（当前在用）

```bash
# 目录
/vol2/1000/esphome-ota/
├── server.py
├── local-inject.js
├── static/index.html
└── .venv/                 # python3 -m venv .venv
                           # .venv/bin/pip install -i 清华源 esphome

# 启动
cd /vol2/1000/esphome-ota
ESPOTA_PORT=8765 ESPOTA_WORK=/vol2/1000/esphome-ota/work \
  HOME=/vol2/1000/esphome-ota/work/home \
  PLATFORMIO_CORE_DIR=/vol2/1000/esphome-ota/work/pio \
  .venv/bin/python3 server.py
```

**注意**：`HOME` 和 `PLATFORMIO_CORE_DIR` 必须指到可写目录，否则 PlatformIO 会尝试写 `/home/<user>/.platformio` 而报 `PermissionError`。

### 开机自启（systemd）

`deploy/esphome-ota.service` 已包含，安装方式：

```bash
sudo cp deploy/esphome-ota.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now esphome-ota
```

> 若改用应用中心安装的 .fpk，请先 `systemctl disable --now esphome-ota`，避免两个实例抢 8765 端口。

## API

| 方法 | 路径 | 说明 |
|---|---|---|
| GET | `/` | 本地页面（ui.html + 本地模式注入） |
| GET | `/api/health` | `{"ok":true,"esphome":"Version: 2026.6.5","work":"..."}` |
| POST | `/api/compile` | `{"yaml":"..."}` → `{"job":"..."}` |
| POST | `/api/ota` | `{"yaml":"...","ip":"192.168.50.203"}` → `{"job":"..."}` |
| GET | `/api/events?job=xxx` | SSE 实时日志，结束发 `{"done":true,"code":0}` |

## 打包

```bash
python build_fpk.py        # 生成 dist/esphome-ota-1.0.0.fpk
```

`.fpk` 结构（与 fn-bing-wallpaper 一致）：

```
manifest            appname/version/platform/os_min_version/checksum(md5 of app.tgz)
config/privilege    {"defaults":{"run-as":"root"}}
config/resource     {}
cmd/main            start/stop/status
cmd/install_callback  创建 venv + 安装 esphome + 启动服务
wizard/install      安装向导（显示访问地址）
app.tgz             应用本体
ICON.PNG / ICON_256.PNG
```

## 已验证

- NAS：飞牛 fnOS（Debian 12 / systemd 252），`/vol2/1000/esphome-ota`
- esphome 2026.6.5（NAS 的 PyPI 只有到 2026.6.5；本机可用 2026.10.x）
- 服务自启：systemd 单元，`Restart=always`
- 页面：`/` 返回 311 KB 的 ui.html，注入脚本在 `readyState==complete` 后探测 `/api/health`

## 许可

MIT


## 已知问题与排查记录

### 1. 飞牛 NAS 走 WiFi，无法访问局域网设备（阻塞 OTA）

NAS 只有无线网卡 `wlp1s0`（192.168.50.11/24），实测：

```
网关 192.168.50.1:80     OPEN
设备 192.168.50.203:8266 FAIL   <- 目标设备
设备 192.168.50.66:80    FAIL
```

同一网段却连不通其它客户端，但能连网关、能被外部 SSH 进来 —— 典型的 **AP 客户端隔离（AP Isolation）** 或 NAS 连的是访客 SSID。

**解决**：在路由器关闭该 SSID 的「AP 隔离/客户端隔离」，或让 NAS 接有线（当前设备无有线网口）。

### 2. PlatformIO 工具链下载极慢

NAS 首次编译需要下载 `toolchain-xtensa`（约 180 MB），实测 10 分钟只下到 1%，ETA 超过 1 小时。可考虑：

- 网络空闲时段预热（应用已支持：先跑一次 compile）
- 或把 `.platformio` 目录从别的 Linux 机器整体拷过来

### 3. 平台目录权限

飞牛的 `$HOME` 不可写，PlatformIO 默认写 `/home/<user>/.platformio` 会报 `PermissionError`。
已通过环境变量解决（server.py 自动设置）：

```
HOME=<work>/home
PLATFORMIO_CORE_DIR=<work>/pio
```

### 4. 已完成验证

| 项目 | 结果 |
|---|---|
| NAS 服务启动 | ✅ systemd 常驻，`active` |
| `/api/health` | ✅ 返回 `Version: 2026.6.5` |
| 页面加载 | ✅ 311 KB，本地面板与「本地模式」徽标正常显示 |
| 编译（Windows 上同一份服务） | ✅ 385,861 字节，37.7% |
| OTA（Windows → 192.168.50.203） | ❌ 目标设备掉线，端口 8266 不可达 |
| OTA（NAS → 设备） | ❌ 被 AP 隔离阻塞 |

> 结论：**服务本身工作正常**，剩下两件事是网络环境（AP 隔离）和目标设备在线状态。

### 5. 测试芯片 ESP-01S 无法关联 WiFi（Auth Expired）

串口日志：

```
Disconnected ssid='ASUS' bssid=40:B0:76:37:8B:19 reason='Auth Expired'
```

试过：WPA2 认证、关闭省电、20dB 发射功率、fast_connect、锁定 BSSID(72:CF:84:2B:F7:21)，均未连上。
同网络的 S30C / CMPOWER 一切正常，因此怀疑该 ESP-01S 供电不足或模块本身异常，需要换一个模块测试。

### 6. HTTPS（8443）

Web Serial 只在安全上下文可用，HTTP 页面点「连接」会提示「这个浏览器不支持 Web Serial」。
服务启动时自动生成自签名证书（SAN 含本机局域网 IP）：

```
http://<NAS>:8765   普通使用 + 本地 OTA
https://<NAS>:8443  串口/Web Serial 可用（证书警告选「继续访问」）
```

## 进度快照

| 项目 | 状态 |
|---|---|
| NAS 服务 + systemd 自启 | ✅ |
| HTTPS 8443 | ✅ |
| .fpk 打包 | ✅ |
| GitHub 仓库 | ✅ https://github.com/txhtxh11/fn-esphome-ota |
| 桌面归档 | ✅ C:\Users\txh11\Desktop\ESPHome-OTA-NAS |
| NAS 工具链预热 | ⏳ 43MB / 180MB |
| 真机 OTA | ⏳ 受阻：AP 隔离 + 测试芯片无法联网 |

### 7. 编译缓存

同一份 YAML 第二次编译直接复用产物（按内容 SHA1 分目录）：

```
首次编译  约 3 分钟
缓存命中  2.5 秒（返回 code 0 + 产物体积）
```

实现：prepare() 用 sha1(yaml)[:12] 作为工作目录名，命中 irmware.ota.bin 就跳过 esphome compile。

## 验证记录（NAS 实测）

```
POST /api/compile
→ INFO Successfully compiled program.
→ 产物 ota: 395760 字节 (386.5 KB)
→ {"done": true, "code": 0}
```

## 真机 OTA 验证（已通过）

网络环境：NAS 与设备之间存在 AP 隔离，NAS 无法直连设备。
用本机做 TCP 代理绕过（NAS 能访问本机 TCP，ICMP 被防火墙拦但不影响）：

```
NAS → 192.168.50.185:8266 ──proxy.py──> 192.168.50.66:8266
```

实测日志：

```
INFO Connected to 192.168.50.185
INFO Uploading .../firmware.bin (659808 bytes)
INFO Compressed to 462950 bytes
INFO Upload took 8.09 seconds
INFO OTA successful
→ {"done": true, "code": 0}
```

OTA 后设备状态：

```
ping 192.168.50.66   通
5556 (HAP)           开
8266 (OTA)           开
巴法 cmpw1006/2006    均在线
```

### 新增接口

| 方法 | 路径 | 说明 |
|---|---|---|
| POST | /api/ota-bin | 直接推送已有 bin（不编译）：{"ip","name","data"(base64),"limit"} |

### 体积上限改为按请求传入

1MB 设备 520 KB、4MB 设备 1.6 MB，由前端按板型传入 limit（字节），默认 520 KB。
