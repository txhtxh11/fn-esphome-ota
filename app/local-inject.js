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

    function panel() {
        var wrap = el("div", [
            "position:fixed", "right:16px", "bottom:16px", "width:330px", "z-index:9999",
            "background:var(--panel,#1b1b1b)", "border:1.5px solid var(--line,#333)",
            "color:var(--text,#eee)", "font:13px/1.5 ui-monospace,monospace",
            "box-shadow:0 8px 32px rgba(0,0,0,.45)", "display:none"
        ].join(";"));
        wrap.id = "espotaLocal";

        var head = el("div", "display:flex;align-items:center;justify-content:space-between;padding:10px 12px;border-bottom:1.5px solid var(--line,#333)");
        head.appendChild(el("span", "font-weight:700", "本地 OTA"));
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
            "background:var(--accent,#f5a524)", "color:#111"
        ].join(";"), "编译并 OTA");
        row.appendChild(go);
        body.appendChild(row);

        var msg = el("div", "font-size:11.5px;opacity:.65", "点一下：本地编译 → 体积校验 → OTA 推送");
        body.appendChild(msg);

        var logBox = el("pre", [
            "margin:0", "max-height:200px", "overflow:auto", "background:#0d0d0d",
            "border:1.5px solid var(--line,#333)", "padding:8px", "font-size:11px",
            "white-space:pre-wrap", "display:none"
        ].join(";"));
        body.appendChild(logBox);
        wrap.appendChild(body);

        go.onclick = function () {
            if (state.busy) return;
            var yaml = "";
            try { yaml = window.__debug ? window.__debug.buildYaml() : ""; } catch (e) { }
            if (!yaml) { msg.textContent = "拿不到 YAML，请先配置设备"; return; }
            state.busy = true;
            go.textContent = "编译中…";
            logBox.style.display = "block";
            logBox.textContent = "";
            fetch(API + "/api/ota", {
                method: "POST",
                headers: { "content-type": "application/json" },
                body: JSON.stringify({ yaml: yaml, ip: state.ip })
            }).then(function (r) { return r.json(); }).then(function (d) {
                if (!d.job) throw new Error(d.error || "no job");
                var es = new EventSource(API + "/api/events?job=" + d.job);
                es.onmessage = function (ev) {
                    var m = JSON.parse(ev.data);
                    if (m.line != null) {
                        logBox.textContent += m.line + "\n";
                        logBox.scrollTop = logBox.scrollHeight;
                    }
                    if (m.done) {
                        es.close();
                        state.busy = false;
                        go.textContent = "编译并 OTA";
                        msg.textContent = m.code === 0 ? "完成" : "失败，看日志";
                    }
                };
                es.onerror = function () { es.close(); state.busy = false; go.textContent = "编译并 OTA"; };
            }).catch(function (e) {
                state.busy = false;
                go.textContent = "编译并 OTA";
                msg.textContent = "请求失败：" + e.message;
            });
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
