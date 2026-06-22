#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>

static const char *AP_SSID = "BalanceCar";
static const char *AP_PASSWORD = "12345678";

ESP8266WebServer server(80);

static void sendCommand(const String &cmd)
{
  Serial.println(cmd);
}

static void handleCommand()
{
  if (!server.hasArg("cmd")) {
    server.send(400, "text/plain", "missing cmd");
    return;
  }

  String cmd = server.arg("cmd");
  cmd.trim();
  if (cmd.length() == 0 || cmd.length() > 24) {
    server.send(400, "text/plain", "bad cmd");
    return;
  }

  sendCommand(cmd);
  server.send(200, "text/plain", "OK");
}

static void handleRoot()
{
  static const char page[] PROGMEM = R"HTML(
<!doctype html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>Balance Car</title>
<style>
html,body{height:100%;margin:0;font-family:system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;background:#101418;color:#eef3f8;touch-action:none}
main{min-height:100%;display:flex;flex-direction:column;gap:18px;align-items:center;justify-content:center;padding:18px;box-sizing:border-box}
h1{font-size:26px;margin:0 0 6px}
.status{height:22px;color:#9fb0c0;font-size:14px}
.grid{display:grid;grid-template-columns:88px 88px 88px;grid-template-rows:72px 72px 72px;gap:12px}
button{border:0;border-radius:8px;background:#26313b;color:#fff;font-size:18px;font-weight:700;box-shadow:inset 0 0 0 1px #394957;user-select:none}
button:active,.active{background:#1f7a52}
.danger{background:#8a2632}
.wide{width:288px;height:58px}
.small{font-size:15px;font-weight:600;color:#c6d0d8}
</style>
</head>
<body>
<main>
  <h1>Balance Car</h1>
  <div class="status" id="status">未发送命令</div>
  <button class="wide" onclick="tap('RUN 1')">启动</button>
  <div class="grid">
    <div></div>
    <button data-down="SPD 0.5" data-up="SPD 0">前进</button>
    <div></div>
    <button data-down="TURN 0.4" data-up="TURN 0">左转</button>
    <button data-down="STOP">停止</button>
    <button data-down="TURN -0.4" data-up="TURN 0">右转</button>
    <div></div>
    <button data-down="SPD -0.5" data-up="SPD 0">后退</button>
    <div></div>
  </div>
  <button class="wide danger" onclick="tap('RUN 0')">急停</button>
  <div class="small">松开方向键后目标会自动归零</div>
</main>
<script>
const statusEl=document.getElementById('status');
function send(cmd){
  statusEl.textContent='发送: '+cmd;
  fetch('/cmd?cmd='+encodeURIComponent(cmd)).catch(()=>{statusEl.textContent='发送失败'});
}
function tap(cmd){ send(cmd); }
document.querySelectorAll('[data-down]').forEach(btn=>{
  let repeat=0;
  const down=()=>{btn.classList.add('active');send(btn.dataset.down);};
  const start=()=>{down();clearInterval(repeat);repeat=setInterval(()=>send(btn.dataset.down),200);};
  const up=()=>{btn.classList.remove('active');clearInterval(repeat);repeat=0;if(btn.dataset.up)send(btn.dataset.up);};
  btn.addEventListener('touchstart',e=>{e.preventDefault();start();},{passive:false});
  btn.addEventListener('touchend',e=>{e.preventDefault();up();},{passive:false});
  btn.addEventListener('touchcancel',e=>{e.preventDefault();up();},{passive:false});
  btn.addEventListener('mousedown',start);
  btn.addEventListener('mouseup',up);
  btn.addEventListener('mouseleave',up);
});
</script>
</body>
</html>
)HTML";

  server.send_P(200, "text/html; charset=utf-8", page);
}

void setup()
{
  Serial.begin(115200);
  Serial.setTimeout(20);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  server.on("/", handleRoot);
  server.on("/cmd", handleCommand);
  server.begin();
}

void loop()
{
  server.handleClient();
}
