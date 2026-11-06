/* 本地模式注入脚本：仅当页面由本机/NAS 服务提供、且 /api/health 可用时启用 */
(function () {
    "use strict";
    var API = "";
    var state = { ip: localStorage.getItem("espota-ip") || "192.168.50.66", busy: false };

    function el(tag, css, text) {
        var e = document.createElement(tag);
        if (css) e.style.cssText = css;
        if (text) e.textContent = text;
        return e;
    }

    function limitForState() {
        try {
            var st = window.__debug && window.__debug.getState ? window.__debug.getState() : null;
            var flash = st && st.flash;
            if (flash === "4MB" || flash === "8MB") return 1600000;
            if (flash === "2MB") return 1000000;
        } catch (e) { }
        return 520000;   // 1MB 设备默认
    }

    function stream(job, logBox, onDone) {
        var es = new EventSource(API + "/api/events?job=" + job);
        es.onmessage = function (ev) {
            var m = JSON.parse(ev.data);
            if (m.line != null) {
                logBox.textContent += m.line + "\n";
                logBox.scrollTop = logBox.scrollHeight;
            }
            if (m.done) {
                es.close();
                onDone(m.code);
            }
        };
        es.onerror = function () { es.close(); onDone(-1); };
    }

    function panel() {
        var wrap = el("div", [
            "position:fixed", "right:16px", "bottom:16px", "width:330px", "z-index:9999",
            "background:var(--panel,#1b1b1b)", "border:1.5px solid var(--line,#333)",
            "color:var(--text,#eee)", "font:13px/1.5 ui-monospace,monospace",
            "box-shadow:0 8px 32px rgba(0,0,0,.45)", "display:none"
        ].join(";"));
        wrap.id = "espotaLocal";

        var st = document.createElement("style");
        st.textContent = "#espotaLocal .hk-hint{display:none;color:var(--accent,#f5a524);" +
            "font-size:10.5px;line-height:1.3;font-weight:600;white-space:nowrap;" +
            "overflow:hidden;text-overflow:ellipsis;max-width:220px}" +
            "#espotaLocal.hk-show .hk-hint{display:inline-block}";
        document.head.appendChild(st);

        var head = el("div", "display:flex;align-items:center;gap:8px;padding:10px 12px;border-bottom:1.5px solid var(--line,#333)");
        var title = el("span", "font-weight:700;cursor:default", "本地 OTA");
        var hint = el("span", "", "本地编译 → 体积校验 → OTA 推送");
        hint.className = "hk-hint";
        var hintTimer = null;
        function showHint(ms) {
            wrap.classList.add("hk-show");
            if (hintTimer) clearTimeout(hintTimer);
            if (ms) hintTimer = setTimeout(function () { wrap.classList.remove("hk-show"); }, ms);
        }
        head.addEventListener("mouseenter", function () { showHint(0); });
        head.addEventListener("mouseleave", function () {
            if (hintTimer) { clearTimeout(hintTimer); hintTimer = null; }
            wrap.classList.remove("hk-show");
        });
        title.addEventListener("click", function () { showHint(3000); });
        head.appendChild(title);
        head.appendChild(hint);
        var spacer = el("span", "flex:1");
        head.appendChild(spacer);
        var close = el("button", "all:unset;cursor:pointer;opacity:.6;padding:0 4px", "×");
        close.onclick = function () { wrap.style.display = "none"; };
        head.appendChild(close);
        wrap.appendChild(head);

        var body = el("div", "padding:10px 12px;display:flex;flex-direction:column;gap:8px");

        // 第一行：IP + 编译并 OTA
        var row = el("div", "display:flex;gap:8px");
        var ip = el("input", "flex:1;min-width:0;background:#111;border:1.5px solid var(--line,#333);color:inherit;padding:8px;font:inherit");
        ip.value = state.ip;
        ip.placeholder = "设备 IP";
        ip.oninput = function () { state.ip = ip.value.trim(); localStorage.setItem("espota-ip", state.ip); };
        row.appendChild(ip);

        var go = el("button", [
            "all:unset", "cursor:pointer", "padding:8px 12px", "font:inherit", "font-weight:700",
            "background:var(--accent,#f5a524)", "color:#111"
        ].join(";"), "编译并 OTA");
        row.appendChild(go);
        body.appendChild(row);

        var msg = el("div", "font-size:11.5px;opacity:.65;display:none");
        function setMsg(text) {
            msg.textContent = text || "";
            msg.style.display = text ? "block" : "none";
        }
        body.appendChild(msg);
        if (!window.isSecureContext) {
            setMsg("HTTP 下 Web Serial 不可用；OTA 不受影响");
        }

        // 第二行：选择本地 bin 直接推送
        var row2 = el("div", "display:flex;gap:8px;align-items:center");
        var fileBtn = el("button", [
            "all:unset", "cursor:pointer", "padding:7px 10px", "font:inherit", "font-size:12px",
            "border:1.5px solid var(--line,#333)", "color:inherit", "white-space:nowrap"
        ].join(";"), "选本地 bin");
        var file = el("input");
        file.type = "file";
        file.accept = ".bin";
        file.style.display = "none";
        var fileName = el("span", "font-size:11.5px;opacity:.6;overflow:hidden;text-overflow:ellipsis;white-space:nowrap", "未选择文件");
        fileBtn.onclick = function () { file.click(); };
        row2.appendChild(fileBtn);
        row2.appendChild(fileName);
        body.appendChild(row2);

        var logBox = el("pre", [
            "margin:0", "max-height:200px", "overflow:auto", "background:#0d0d0d",
            "border:1.5px solid var(--line,#333)", "padding:8px", "font-size:11px",
            "white-space:pre-wrap", "display:none"
        ].join(";"));
        body.appendChild(logBox);
        wrap.appendChild(body);

        function busyOn(text) {
            state.busy = true;
            go.textContent = text;
            logBox.style.display = "block";
            logBox.textContent = "";
        }
        function busyOff(text) {
            state.busy = false;
            go.textContent = "编译并 OTA";
            setMsg(text);
        }

        go.onclick = function () {
            if (state.busy) return;
            var yaml = "";
            try { yaml = window.__debug ? window.__debug.buildYaml() : ""; } catch (e) { }
            if (!yaml) { setMsg("拿不到 YAML，请先配置设备"); return; }
            busyOn("编译中…");
            fetch(API + "/api/ota", {
                method: "POST",
                headers: { "content-type": "application/json" },
                body: JSON.stringify({ yaml: yaml, ip: state.ip, limit: limitForState() })
            }).then(function (r) { return r.json(); }).then(function (d) {
                if (!d.job) throw new Error(d.error || "no job");
                stream(d.job, logBox, function (code) {
                    busyOff(code === 0 ? "完成" : "失败，看日志");
                });
            }).catch(function (e) {
                busyOff("请求失败：" + e.message);
            });
        };

        file.onchange = function () {
            var f = file.files && file.files[0];
            if (!f) return;
            if (state.busy) { setMsg("正在忙，请稍候"); return; }
            fileName.textContent = f.name + " (" + (f.size / 1024).toFixed(0) + " KB)";
            var limit = limitForState();
            if (f.size > limit) {
                setMsg("文件超过上限 " + (limit / 1024).toFixed(0) + " KB，已拒绝");
                logBox.style.display = "block";
                logBox.textContent = "体积 " + f.size + " 字节 > " + limit + "，拒绝推送\n";
                return;
            }
            var reader = new FileReader();
            reader.onload = function () {
                var b64 = String(reader.result).split(",")[1] || "";
                busyOn("推送中…");
                fetch(API + "/api/ota-bin", {
                    method: "POST",
                    headers: { "content-type": "application/json" },
                    body: JSON.stringify({ ip: state.ip, name: f.name, data: b64, limit: limit })
                }).then(function (r) { return r.json(); }).then(function (d) {
                    if (!d.job) throw new Error(d.error || "no job");
                    stream(d.job, logBox, function (code) {
                        busyOff(code === 0 ? "bin 推送完成" : "bin 推送失败，看日志");
                    });
                }).catch(function (e) {
                    busyOff("请求失败：" + e.message);
                });
            };
            reader.onerror = function () { setMsg("读取文件失败"); };
            reader.readAsDataURL(f);
        };

        return wrap;
    }

    function boot() {
        fetch(API + "/api/health", { cache: "no-store" })
            .then(function (r) { return r.ok ? r.json() : null; })
            .then(function (d) {
                if (!d || !d.ok) return;
                var p = panel();
                document.body.appendChild(p);
                p.style.display = "block";
                var badge = el("div", [
                    "position:fixed", "right:16px", "bottom:" + (p.offsetHeight + 24) + "px",
                    "z-index:9998", "padding:6px 10px", "font:11px ui-monospace,monospace",
                    "background:var(--panel,#1b1b1b)", "border:1.5px solid var(--line,#333)",
                    "color:var(--text,#eee)", "cursor:pointer", "opacity:.85"
                ].join(";"), "本地模式 · esphome " + (d.esphome || "未安装"));
                badge.onclick = function () { p.style.display = p.style.display === "none" ? "block" : "none"; };
                document.body.appendChild(badge);
            })
            .catch(function () { /* 线上版没有 /api，静默忽略 */ });
    }

    if (document.readyState === "complete") setTimeout(boot, 400);
    else window.addEventListener("load", function () { setTimeout(boot, 400); });
})();
