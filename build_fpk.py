#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""把 fnos-app 打包成飞牛 .fpk 安装包（tar 格式，与 fn-bing-wallpaper 同结构）"""

import hashlib
import io
import json
import os
import shutil
import subprocess
import sys
import tarfile

ROOT = os.path.dirname(os.path.abspath(__file__))
APP = os.path.join(ROOT, "app")
OUT = os.path.join(ROOT, "dist")
NAME = "esphome-ota"
VERSION = "1.0.0"
DISPLAY = "ESPHome OTA 工具"
DESC = "在飞牛 NAS 上本地编译 ESPHome 固件并直接 OTA 推送到局域网设备；带体积校验（1MB 设备超限拦截）与实时日志。"
MAINTAINER = "TXH"
MIN_OS = "1.1.8"
PORT = 8765
MIRROR = "https://pypi.tuna.tsinghua.edu.cn/simple"
ESPHOME_VER = "2026.6.5"

MANIFEST = f"""appname                    = {NAME}
version                    = {VERSION}
display_name               = {DISPLAY}
desc                       = {DESC}
platform                   = x86
source                     = thirdparty
maintainer                 = {MAINTAINER}
maintainer_url             = https://github.com/txhtxh11
distributor                = {MAINTAINER}
distributor_url            = https://txhtxh11.github.io
ctl_stop                   = true
checkport                  = false
os_min_version             = {MIN_OS}
disable_authorization_path = true
changelog                  = 1.0.0 首个版本
"""

CMD_MAIN = f"""#!/bin/bash
APP_DIR="${{TRIM_APPDEST}}"
VAR_DIR="${{TRIM_PKGVAR}}"
PIDFILE="$VAR_DIR/server.pid"
LOG="$VAR_DIR/server.log"
PORT={PORT}

start() {{
  mkdir -p "$VAR_DIR/work"
  if [ -f "$PIDFILE" ] && kill -0 "$(cat "$PIDFILE")" 2>/dev/null; then
    exit 0
  fi
  cd "$APP_DIR" || exit 1
  PY="$APP_DIR/.venv/bin/python3"
  [ -x "$PY" ] || PY="python3"
  ESPOTA_PORT="$PORT" ESPOTA_TLS_PORT="${{ESPOTA_TLS_PORT:-8443}}" ESPOTA_WORK="$VAR_DIR/work" \
    setsid nohup "$PY" server.py >>"$LOG" 2>&1 < /dev/null &
  echo $! > "$PIDFILE"
  exit 0
}}

stop() {{
  if [ -f "$PIDFILE" ]; then
    kill "$(cat "$PIDFILE")" 2>/dev/null
    rm -f "$PIDFILE"
  fi
  pkill -f "$APP_DIR/server.py" 2>/dev/null
  exit 0
}}

status() {{
  if [ -f "$PIDFILE" ] && kill -0 "$(cat "$PIDFILE")" 2>/dev/null; then
    exit 0
  fi
  exit 1
}}

case "$1" in
  start) start ;;
  stop) stop ;;
  status) status ;;
  *) exit 1 ;;
esac
"""

INSTALL_CB = f"""#!/bin/bash
APP_DIR="${{TRIM_APPDEST}}"
VAR_DIR="${{TRIM_PKGVAR}}"
LOG="$VAR_DIR/install.log"

log() {{ echo "$(date '+%Y-%m-%d %H:%M:%S') $1" >> "$LOG"; }}

mkdir -p "$VAR_DIR/work"
chmod +x "$APP_DIR/server.py" 2>/dev/null || true

log "开始安装 ESPHome OTA 服务"
if [ ! -x "$APP_DIR/.venv/bin/esphome" ]; then
  log "创建 Python 虚拟环境"
  python3 -m venv "$APP_DIR/.venv" >> "$LOG" 2>&1
  log "安装 esphome（清华源，约 5-15 分钟）"
  "$APP_DIR/.venv/bin/pip" install -i {MIRROR} --retries 10 --timeout 60 --no-input "esphome=={ESPHOME_VER}" >> "$LOG" 2>&1
  log "esphome 安装结束，返回码 $?"
else
  log "esphome 已存在，跳过安装"
fi

"$APP_DIR/cmd/main" start >> "$LOG" 2>&1
log "服务已启动，端口 {PORT}"
exit 0
"""

UNINSTALL_CB = """#!/bin/bash
APP_DIR="${TRIM_APPDEST}"
VAR_DIR="${TRIM_PKGVAR}"
"$APP_DIR/cmd/main" stop 2>/dev/null || true
rm -f "$VAR_DIR/server.pid"
exit 0
"""

CONFIG_CB = """#!/bin/bash
# 配置变更后重启服务
APP_DIR="${TRIM_APPDEST}"
"$APP_DIR/cmd/main" stop 2>/dev/null || true
sleep 1
"$APP_DIR/cmd/main" start 2>/dev/null || true
exit 0
"""

SIMPLE_INIT = "#!/bin/bash\n\nexit 0\n"

WIZARD = json.dumps([
    {
        "stepTitle": "说明",
        "items": [
            {
                "type": "text",
                "field": "wizard_note",
                "label": "安装后访问地址",
                "initValue": f"http://<NAS-IP>:{PORT}",
                "helpText": "安装过程会在后台创建 Python 环境并安装 esphome（约 5-15 分钟）。安装完成后用浏览器打开上面的地址即可使用本地 OTA。",
                "rules": [],
            }
        ],
    }
], ensure_ascii=False, indent=4)


def make_icons(outdir):
    try:
        from PIL import Image, ImageDraw
    except Exception:
        return False
    for size, fname in ((256, "ICON_256.PNG"), (128, "ICON.PNG")):
        img = Image.new("RGB", (size, size), (18, 18, 18))
        d = ImageDraw.Draw(img)
        pad = size // 8
        d.rounded_rectangle([pad, pad, size - pad, size - pad], radius=size // 10, fill=(245, 165, 36))
        txt = "OTA"
        fs = size // 4
        try:
            from PIL import ImageFont
            font = ImageFont.truetype("arial.ttf", fs)
        except Exception:
            font = None
        bbox = d.textbbox((0, 0), txt, font=font)
        w, h = bbox[2] - bbox[0], bbox[3] - bbox[1]
        d.text(((size - w) / 2 - bbox[0], (size - h) / 2 - bbox[1]), txt, fill=(18, 18, 18), font=font)
        img.save(os.path.join(outdir, fname))
    return True


def build():
    if os.path.exists(OUT):
        shutil.rmtree(OUT)
    os.makedirs(OUT)
    pkg = os.path.join(OUT, "pkg")
    os.makedirs(pkg)

    # 目录结构
    for d in ("config", "cmd", "wizard"):
        os.makedirs(os.path.join(pkg, d), exist_ok=True)

    def w(rel, text, mode=0o755):
        p = os.path.join(pkg, rel)
        with open(p, "w", encoding="utf-8", newline="\n") as f:
            f.write(text)
        os.chmod(p, mode)

    w("config/privilege", json.dumps({"defaults": {"run-as": "root"}}, indent=4), 0o644)
    w("config/resource", "{}", 0o644)
    w("cmd/main", CMD_MAIN)
    w("cmd/install_init", SIMPLE_INIT)
    w("cmd/install_callback", INSTALL_CB)
    w("cmd/uninstall_init", SIMPLE_INIT)
    w("cmd/uninstall_callback", UNINSTALL_CB)
    w("cmd/upgrade_init", SIMPLE_INIT)
    w("cmd/upgrade_callback", INSTALL_CB)
    w("cmd/config_init", SIMPLE_INIT)
    w("cmd/config_callback", CONFIG_CB)
    w("wizard/install", WIZARD, 0o644)

    # app.tgz：应用本体
    apptgz = os.path.join(pkg, "app.tgz")
    with tarfile.open(apptgz, "w:gz") as t:
        def add(path, arc):
            t.add(path, arcname=arc, filter=lambda ti: setattr(ti, "uid", 0) or setattr(ti, "gid", 0) or ti)
        add(os.path.join(APP, "server.py"), "server.py")
        add(os.path.join(APP, "local-inject.js"), "local-inject.js")
        add(os.path.join(APP, "static"), "static")

    md5 = hashlib.md5(open(apptgz, "rb").read()).hexdigest()
    w("manifest", MANIFEST + f"checksum                   = {md5}\n", 0o644)

    icons_ok = make_icons(pkg)
    if not icons_ok:
        print("警告: 未能生成图标（缺 Pillow）")

    fpk = os.path.join(OUT, f"{NAME}-{VERSION}.fpk")
    with tarfile.open(fpk, "w") as t:
        for base, dirs, files in os.walk(pkg):
            for name in sorted(files):
                full = os.path.join(base, name)
                rel = os.path.relpath(full, pkg).replace("\\", "/")
                ti = t.gettarinfo(full, arcname="./" + rel)
                ti.uid = ti.gid = 0
                ti.uname = ti.gname = "root"
                with open(full, "rb") as f:
                    t.addfile(ti, f)
    print("生成:", fpk, os.path.getsize(fpk), "字节")
    return fpk


if __name__ == "__main__":
    build()
