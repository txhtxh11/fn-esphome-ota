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
                var clean = String(m.line).replace(/\x1b\[[0-9;]*[A-Za-z]/g, "");
                logBox.textContent += clean + "\n";
                logBox.scrollTop = logBox.scrollHeight;
            }
            if (m.done) {
                es.close();
                onDone(m);
            }
        };
        es.onerror = function () { es.close(); onDone({ code: -1 }); };
    }

    function panel() {
        var wrap = el("div", [
            "position:fixed", "right:16px", "bottom:16px", "width:330px", "z-index:9999",
            "background:var(--panel,#1b1b1b)", "border:1.5px solid var(--line,#333)",
            "color:var(--text,#eee)", "font:13px/1.5 ui-monospace,monospace",
            "box-shadow:0 8px 32px rgba(0,0,0,.45)", "display:none"
        ].join(";"));
        wrap.id = "espotaLocal";

        var styleTag = document.createElement("style");
        styleTag.textContent =
            "#espotaLocal .hk-hint{display:none;color:var(--accent,#f5a524);font-size:10.5px;" +
            "line-height:1.3;font-weight:600;white-space:nowrap;overflow:hidden;" +
            "text-overflow:ellipsis;max-width:200px}" +
            "#espotaLocal.hk-show .hk-hint{display:inline-block}" +
            "#espotaLocal .hk-warn{color:#f5a524}" +
            "#espotaLocal .hk-force{all:unset;cursor:pointer;color:var(--accent,#f5a524);" +
            "text-decoration:underline;font-weight:700}" +
            "#espotaLocal .hk-log{scrollbar-width:none;-ms-overflow-style:none;cursor:text}" +
            "#espotaLocal .hk-log::-webkit-scrollbar{width:0;height:0;display:none}";
        document.head.appendChild(styleTag);

        var head = el("div", "display:flex;align-items:center;gap:8px;padding:10px 12px;border-bottom:1.5px solid var(--line,#333)");
        head.appendChild(el("span", "font-weight:700", "本地 OTA"));
        var hint = el("span", "", "本地编译 → 体积校验 → OTA 推送");
        hint.className = "hk-hint";
        head.appendChild(hint);
        var hintTimer = null;
        function showHint(ms) {
            wrap.classList.add("hk-show");
            if (hintTimer) clearTimeout(hintTimer);
            if (ms) hintTimer = setTimeout(function () { wrap.classList.remove("hk-show"); }, ms);
        }
        head.appendChild(el("span", "flex:1"));
        var close = el("button", "all:unset;cursor:pointer;opacity:.6;padding:0 4px", "×");
        close.onclick = function () { wrap.style.display = "none"; };
        head.appendChild(close);
        wrap.appendChild(head);

        var body = el("div", "padding:10px 12px;display:flex;flex-direction:column;gap:8px");

        var row = el("div", "display:flex;gap:8px");
        var ip = el("input", "flex:1;min-width:0;background:#111;border:1.5px solid var(--line,#333);color:inherit;padding:8px;font:inherit");
        ip.value = state.ip;
        ip.placeholder = "设备 IP";
        ip.oninput = function () { state.ip = ip.value.trim(); localStorage.setItem("espota-ip", state.ip); };
        row.appendChild(ip);

        var go = el("button", [
            "all:unset", "cursor:pointer", "padding:8px 12px", "font:inherit", "font-weight:700",
            "background:var(--accent,#f5a524)", "color:#111", "white-space:nowrap"
        ].join(";"), "编译并 OTA");
        go.addEventListener("mouseenter", function () { showHint(0); });
        go.addEventListener("mouseleave", function () {
            if (hintTimer) { clearTimeout(hintTimer); hintTimer = null; }
            wrap.classList.remove("hk-show");
        });
        go.addEventListener("click", function () { showHint(3000); });
        row.appendChild(go);
        body.appendChild(row);

        var msg = el("div", "font-size:11.5px;opacity:.75;display:none");
        function setMsg(text, cls) {
            msg.textContent = text || "";
            msg.className = cls === "warn" ? "hk-warn" : "";
            msg.style.display = text ? "block" : "none";
        }
        body.appendChild(msg);

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
            "margin:0", "max-height:200px", "overflow-y:auto", "overflow-x:hidden",
            "background:#0d0d0d", "border:1.5px solid var(--line,#333)", "padding:8px",
            "font-size:11px", "white-space:pre-wrap", "word-break:break-all",
            "overflow-wrap:anywhere", "display:none"
        ].join(";"));
        logBox.className = "hk-log";   // 隐藏滚动条，滚轮照常滚动
        body.appendChild(logBox);
        wrap.appendChild(body);

        function busyOn(text) {
            state.busy = true;
            go.textContent = text;
            logBox.style.display = "block";
            logBox.textContent = "";
        }
        function busyOff(text, cls) {
            state.busy = false;
            go.textContent = "编译并 OTA";
            setMsg(text, cls);
        }

        function offerForce(payload, url, m) {
            busyOff("超出建议上限（" + Math.round(m.size / 1024) + " KB > " +
                Math.round(m.limit / 1024) + " KB）。设备实际空间可能够用，可继续推送。", "warn");
            var f = el("button", "", "仍要推送");
            f.className = "hk-force";
            f.onclick = function () {
                var p = Object.assign({}, payload, { force: true });
                start(p, url);
            };
            msg.appendChild(document.createTextNode(" "));
            msg.appendChild(f);
        }

        function start(payload, url) {
            if (state.busy) return;
            busyOn(url === "/api/ota" ? "编译中…" : "推送中…");
            fetch(API + url, {
                method: "POST",
                headers: { "content-type": "application/json" },
                body: JSON.stringify(payload)
            }).then(function (r) { return r.json(); }).then(function (d) {
                if (!d.job) throw new Error(d.error || "no job");
                stream(d.job, logBox, function (m) {
                    if (m.overLimit) { offerForce(payload, url, m); return; }
                    if (url === "/api/ota") {
                        busyOff(m.code === 0 ? "完成" : "失败，看日志", m.code === 0 ? "" : "warn");
                    } else {
                        busyOff(m.code === 0 ? "bin 推送完成" : "bin 推送失败，看日志", m.code === 0 ? "" : "warn");
                    }
                });
            }).catch(function (e) {
                busyOff("请求失败：" + e.message, "warn");
            });
        }

        go.onclick = function () {
            var yaml = "";
            try { yaml = window.__debug ? window.__debug.buildYaml() : ""; } catch (e) { }
            if (!yaml) { setMsg("拿不到 YAML，请先配置设备", "warn"); return; }
            start({ yaml: yaml, ip: state.ip, limit: limitForState() }, "/api/ota");
        };

        file.onchange = function () {
            var f = file.files && file.files[0];
            if (!f) return;
            fileName.textContent = f.name + " (" + (f.size / 1024).toFixed(0) + " KB)";
            var reader = new FileReader();
            reader.onload = function () {
                var b64 = String(reader.result).split(",")[1] || "";
                start({ ip: state.ip, name: f.name, data: b64, limit: limitForState() }, "/api/ota-bin");
            };
            reader.onerror = function () { setMsg("读取文件失败", "warn"); };
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
            .catch(function () { });
    }

    if (document.readyState === "complete") setTimeout(boot, 400);
    else window.addEventListener("load", function () { setTimeout(boot, 400); });
})();
