"""本机 TCP 代理：NAS(或任何机器) → 本机:8266 → 设备:8266。

NAS 通过 AP 隔离无法直接访问设备，但能访问本机（445 可达验证）。
在 NAS 上用 `esphome upload --device <本机IP> --file fw.bin cfg.yaml`，
流量会经由这里转发到设备。
"""

import select
import socket
import sys
import threading

LISTEN = ("0.0.0.0", int(sys.argv[1]) if len(sys.argv) > 1 else 8266)
TARGET = (sys.argv[2] if len(sys.argv) > 2 else "192.168.50.66", int(sys.argv[3]) if len(sys.argv) > 3 else 8266)


def pipe(a, b):
    try:
        while True:
            r, _, _ = select.select([a, b], [], [], 60)
            if not r:
                break
            if a in r:
                data = a.recv(4096)
                if not data:
                    break
                b.sendall(data)
            if b in r:
                data = b.recv(4096)
                if not data:
                    break
                a.sendall(data)
    except Exception:
        pass
    finally:
        for s in (a, b):
            try:
                s.close()
            except Exception:
                pass


def handle(conn, addr):
    print("连接来自 %s:%d" % addr, flush=True)
    try:
        up = socket.create_connection(TARGET, timeout=10)
    except Exception as e:
        print("目标不可达 %s: %s" % (TARGET, e), flush=True)
        conn.close()
        return
    pipe(conn, up)


def main():
    srv = socket.socket()
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(LISTEN)
    srv.listen(8)
    print("代理监听 %s:%d -> %s:%d" % (LISTEN[0], LISTEN[1], TARGET[0], TARGET[1]), flush=True)
    while True:
        conn, addr = srv.accept()
        threading.Thread(target=handle, args=(conn, addr), daemon=True).start()


if __name__ == "__main__":
    main()
