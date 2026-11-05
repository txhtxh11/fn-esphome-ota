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
