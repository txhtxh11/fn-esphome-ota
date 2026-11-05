#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ESPHome 本地 OTA 服务（飞牛 NAS）
只用标准库，Python 3.8+ 可运行。

功能：
  GET  /                 -> 返回页面（static/index.html）
  GET  /api/health       -> {"ok":true,"esphome":"x.y.z"|null}
  POST /api/compile      -> 编译 YAML，返回体积与日志
  POST /api/ota          -> 编译并 OTA 推送到设备
  GET  /api/events?job=  -> SSE 实时日志
一切都在本地执行，设备必须与服务在同一内网。
"""

import json
import os
import re
import queue
import shutil
import subprocess
import sys
import tempfile
import threading
import time
import uuid
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse, parse_qs

BASE = os.path.dirname(os.path.abspath(__file__))
STATIC = os.path.join(BASE, "static")
WORK = os.environ.get("ESPOTA_WORK", os.path.join(BASE, "work"))
PORT = int(os.environ.get("ESPOTA_PORT", "8765"))
OTA_LIMIT_1M = 520 * 1024          # 1MB 设备的 OTA 槽上限，超过直接拒绝

JOBS = {}          # job_id -> {"lines": [...], "done": bool, "code": int}
JOBS_LOCK = threading.Lock()


def log(job, text):
    with JOBS_LOCK:
        J = JOBS.setdefault(job, {"lines": [], "done": False, "code": None})
        for line in text.splitlines():
            J["lines"].append(line)
    print(text, flush=True)


def finish(job, code):
    with JOBS_LOCK:
        J = JOBS.setdefault(job, {"lines": [], "done": False, "code": None})
        J["done"] = True
        J["code"] = code


def which_esphome():
    """返回可执行的命令前缀列表：['esphome'] 或 [python, '-m', 'esphome']"""
    for c in (os.environ.get("ESPHOME_BIN"), shutil.which("esphome"), os.path.join(BASE, ".venv", "bin", "esphome")):
        if c and os.path.exists(c):
            return [c]
    try:
        subprocess.check_output([sys.executable, "-m", "esphome", "version"], timeout=30,
                                stderr=subprocess.DEVNULL)
        return [sys.executable, "-m", "esphome"]
    except Exception:
        return None


def tool_env():
    """平台目录放到工作区，避免 NAS 上 $HOME 不可写导致的 PermissionError"""
    env = dict(os.environ)
    home = os.path.join(WORK, "home")
    pio = os.path.join(WORK, "pio")
    os.makedirs(home, exist_ok=True)
    os.makedirs(pio, exist_ok=True)
    env["HOME"] = home
    env["PLATFORMIO_CORE_DIR"] = pio
    return env


def run(job, args, cwd=None, env=None):
    log(job, "$ " + " ".join(args))
    try:
        p = subprocess.Popen(args, cwd=cwd, env=env, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT, bufsize=1, universal_newlines=True)
    except Exception as e:
        log(job, "启动失败: %s" % e)
        return 127
    for line in p.stdout:
        log(job, line.rstrip())
    return p.wait()


def prepare(yaml_text):
    os.makedirs(WORK, exist_ok=True)
    d = os.path.join(WORK, "build-" + uuid.uuid4().hex[:8])
    os.makedirs(d, exist_ok=True)
    path = os.path.join(d, "device.yaml")
    with open(path, "w", encoding="utf-8") as f:
        f.write(yaml_text)
    return d, path


def node_name(yaml_text):
    m = re.search(r"^\s{2}name:\s*[\"']?([^\"'\s]+)", yaml_text, re.M)
    return m.group(1) if m else "device"


def do_build(job, yaml_text, upload_ip=None, bin_kind="bin"):
    esphome = which_esphome()
    if not esphome:
        log(job, "未找到 esphome，请先在本服务目录执行 pip install esphome")
        return 2
    d, path = prepare(yaml_text)
    log(job, "工作目录: " + d)
    env = tool_env()
    code = run(job, esphome + ["compile", path], cwd=d, env=env)
    if code != 0:
        log(job, "编译失败 (code=%d)" % code)
        return code

    name = node_name(yaml_text)
    outdir = os.path.join(d, ".esphome", "build", name, ".pioenvs", name)
    cand = {
        "bin": os.path.join(outdir, "firmware.bin"),
        "ota": os.path.join(outdir, "firmware.ota.bin"),
        "factory": os.path.join(outdir, "firmware.factory.bin"),
    }
    size = 0
    for k in ("ota", "bin", "factory"):
        if os.path.exists(cand[k]):
            size = os.path.getsize(cand[k])
            log(job, "产物 %s: %d 字节 (%.1f KB)" % (k, size, size / 1024.0))
            break
    if 0 < size > OTA_LIMIT_1M:
        log(job, "体积超限 %d > %d，已拒绝推送（1MB 设备会写坏）" % (size, OTA_LIMIT_1M))
        return 3

    if upload_ip:
        fw = cand["ota"] if os.path.exists(cand["ota"]) else cand["bin"]
        log(job, "准备 OTA 到 %s" % upload_ip)
        code = run(job, esphome + ["upload", "--device", upload_ip, "--file", fw, path], cwd=d, env=env)
        log(job, "OTA 成功" if code == 0 else "OTA 失败 (code=%d)" % code)
        return code
    return 0


class Handler(BaseHTTPRequestHandler):
    server_version = "espota/1.0"

    def _send(self, code, body, ctype="application/json; charset=utf-8"):
        data = body.encode("utf-8") if isinstance(body, str) else body
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Headers", "content-type")
        self.send_header("Access-Control-Allow-Methods", "GET,POST,OPTIONS")
        self.end_headers()
        self.wfile.write(data)

    def log_message(self, fmt, *args):
        pass

    def do_OPTIONS(self):
        self._send(204, b"")

    def do_GET(self):
        u = urlparse(self.path)
        if u.path in ("/", "/index.html"):
            idx = os.path.join(STATIC, "index.html")
            if not os.path.exists(idx):
                return self._send(404, "index.html 缺失", "text/plain; charset=utf-8")
            with open(idx, "rb") as f:
                return self._send(200, f.read(), "text/html; charset=utf-8")
        if u.path == "/api/health":
            e = which_esphome()
            ver = None
            if e:
                try:
                    ver = subprocess.check_output(e + ["version"], timeout=30).decode().strip().splitlines()[-1]
                except Exception:
                    ver = "unknown"
            return self._send(200, json.dumps({"ok": True, "esphome": ver, "work": WORK}))
        if u.path == "/api/events":
            job = (parse_qs(u.query).get("job") or [""])[0]
            return self._stream(job)
        # 静态文件：/local-inject.js 之类
        rel = u.path.lstrip("/")
        if rel and ".." not in rel:
            for base in (STATIC, BASE):
                fp = os.path.join(base, rel)
                if os.path.isfile(fp):
                    ctype = "application/octet-stream"
                    if fp.endswith(".js"):
                        ctype = "application/javascript; charset=utf-8"
                    elif fp.endswith(".css"):
                        ctype = "text/css; charset=utf-8"
                    elif fp.endswith(".html"):
                        ctype = "text/html; charset=utf-8"
                    elif fp.endswith(".png"):
                        ctype = "image/png"
                    with open(fp, "rb") as f:
                        return self._send(200, f.read(), ctype)
        return self._send(404, json.dumps({"error": "not found"}))

    def _stream(self, job):
        self.send_response(200)
        self.send_header("Content-Type", "text/event-stream; charset=utf-8")
        self.send_header("Cache-Control", "no-cache")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        sent = 0
        idle = 0
        try:
            while True:
                with JOBS_LOCK:
                    J = JOBS.get(job)
                    lines = J["lines"][sent:] if J else []
                    done = bool(J and J["done"])
                    code = J["code"] if J else None
                for ln in lines:
                    self.wfile.write(("data: " + json.dumps({"line": ln}, ensure_ascii=False) + "\n\n").encode("utf-8"))
                    sent += 1
                if done:
                    self.wfile.write(("data: " + json.dumps({"done": True, "code": code}) + "\n\n").encode("utf-8"))
                    break
                idle = 0 if lines else idle + 1
                if idle > 1200:
                    self.wfile.write(b"data: {\"timeout\":true}\n\n")
                    break
                time.sleep(0.25)
                self.wfile.flush()
        except Exception:
            pass

    def do_POST(self):
        u = urlparse(self.path)
        n = int(self.headers.get("content-length") or 0)
        raw = self.rfile.read(n) if n else b"{}"
        try:
            body = json.loads(raw or b"{}")
        except Exception:
            return self._send(400, json.dumps({"error": "bad json"}))

        if u.path in ("/api/compile", "/api/ota"):
            yaml_text = body.get("yaml")
            if isinstance(yaml_text, dict):
                yaml_text = yaml_text.get("yaml")
            if not isinstance(yaml_text, str):
                yaml_text = ""
            if not yaml_text.strip():
                return self._send(400, json.dumps({"error": "empty yaml"}))
            ip = body.get("ip") if u.path == "/api/ota" else None
            job = uuid.uuid4().hex[:12]
            with JOBS_LOCK:
                JOBS[job] = {"lines": [], "done": False, "code": None}

            def worker():
                try:
                    code = do_build(job, yaml_text, ip)
                except Exception as e:
                    log(job, "异常: %s" % e)
                    code = 1
                finish(job, code)

            threading.Thread(target=worker, daemon=True).start()
            return self._send(200, json.dumps({"job": job}))
        return self._send(404, json.dumps({"error": "not found"}))


def main():
    os.makedirs(WORK, exist_ok=True)
    srv = ThreadingHTTPServer(("0.0.0.0", PORT), Handler)
    print("ESPHome OTA 本地服务已启动: http://0.0.0.0:%d  (工作目录 %s)" % (PORT, WORK), flush=True)
    srv.serve_forever()


if __name__ == "__main__":
    main()
