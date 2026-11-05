"""反向隧道：把 NAS 的 8266 端口转发到本机能访问的设备 8266。
用途：绕开 NAS 的 AP 隔离（NAS 连不到局域网设备，但本机能连）。

运行后，在 NAS 上执行：
    esphome upload --device 127.0.0.1 --file firmware.bin config.yaml
就会经由这条隧道推到设备。
"""

import select
import socket
import sys
import threading
import time

import paramiko

NAS_HOST = "192.168.50.11"
NAS_USER = "t"
NAS_PASS = "t8916090"
TARGET = sys.argv[1] if len(sys.argv) > 1 else "192.168.50.66"
TARGET_PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 8266
LISTEN_PORT = int(sys.argv[3]) if len(sys.argv) > 3 else 8266


class ForwardServer(threading.Thread):
    def __init__(self, transport, host, port, target, target_port):
        super().__init__(daemon=True)
        self.transport = transport
        self.host = host
        self.port = port
        self.target = target
        self.target_port = target_port

    def run(self):
        self.transport.request_port_forward(self.host, self.port)
        print("隧道已建立: NAS:%d -> %s:%d" % (self.port, self.target, self.target_port), flush=True)
        while True:
            chan = self.transport.accept(1000)
            if chan is None:
                continue
            threading.Thread(target=self.handle, args=(chan,), daemon=True).start()

    def handle(self, chan):
        try:
            sock = socket.create_connection((self.target, self.target_port), timeout=10)
        except Exception as e:
            print("目标不可达:", e, flush=True)
            chan.close()
            return
        print("连接: %s:%d" % (self.target, self.target_port), flush=True)
        try:
            while True:
                r, _, _ = select.select([chan, sock], [], [], 30)
                if not r:
                    break
                if chan in r:
                    data = chan.recv(4096)
                    if not data:
                        break
                    sock.sendall(data)
                if sock in r:
                    data = sock.recv(4096)
                    if not data:
                        break
                    chan.sendall(data)
        except Exception:
            pass
        finally:
            try:
                chan.close()
            except Exception:
                pass
            try:
                sock.close()
            except Exception:
                pass


def main():
    t = paramiko.Transport((NAS_HOST, 22))
    t.connect(username=NAS_USER, password=NAS_PASS)
    srv = ForwardServer(t, "127.0.0.1", LISTEN_PORT, TARGET, TARGET_PORT)
    srv.start()
    while True:
        time.sleep(5)


if __name__ == "__main__":
    main()
