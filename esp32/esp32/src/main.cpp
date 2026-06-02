#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

namespace {

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

constexpr char kApSsid[] = "Tennis_Robot";
constexpr char kApPassword[] = "12345678";
constexpr uint32_t kDebugBaudRate = 115200;
constexpr uint32_t kSerial2BaudRate = 115200;
constexpr uint32_t kHeartbeatIntervalMs = 500;
constexpr uint8_t kManualModeCode = 0x06;
constexpr uint8_t kPickupModeCode = 0x07;
constexpr uint8_t kSerial2RxPin = 16;
constexpr uint8_t kSerial2TxPin = 17;
constexpr uint8_t kPacketHeader = 0xFF;
constexpr uint8_t kPacketTail = 0xFE;

const char kControlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0, viewport-fit=cover" />
  <title>自动网球拾取机器人</title>
  <style>
    :root {
      color-scheme: dark;
      --bg: #050816;
      --panel: rgba(18, 27, 50, 0.9);
      --panel-border: rgba(95, 140, 255, 0.28);
      --primary: #4da3ff;
      --primary-strong: #73b9ff;
      --danger: #ff6b6b;
      --ok: #2ee6a6;
      --text: #e8f1ff;
      --muted: #97a6c6;
      --shadow: 0 20px 50px rgba(0, 0, 0, 0.35);
      --button-size: min(24vw, 104px);
      --gap: min(5vw, 20px);
    }

    * {
      box-sizing: border-box;
      -webkit-tap-highlight-color: transparent;
      user-select: none;
    }

    body {
      margin: 0;
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 20px;
      background:
        radial-gradient(circle at top, rgba(77, 163, 255, 0.18), transparent 34%),
        radial-gradient(circle at bottom, rgba(46, 230, 166, 0.12), transparent 26%),
        var(--bg);
      color: var(--text);
      font-family: "Segoe UI", "PingFang SC", "Microsoft YaHei", sans-serif;
    }

    .panel {
      width: min(100%, 420px);
      padding: 24px 20px 28px;
      border: 1px solid var(--panel-border);
      border-radius: 24px;
      background: var(--panel);
      box-shadow: var(--shadow);
      backdrop-filter: blur(16px);
    }

    .title {
      margin: 0;
      font-size: clamp(1.4rem, 4vw, 1.9rem);
      font-weight: 700;
      letter-spacing: 0.04em;
      text-align: center;
    }

    .subtitle {
      margin: 10px 0 0;
      color: var(--muted);
      text-align: center;
      font-size: 0.95rem;
    }

    .status {
      margin-top: 20px;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 10px;
      padding: 12px 16px;
      border-radius: 16px;
      background: rgba(255, 255, 255, 0.04);
      border: 1px solid rgba(255, 255, 255, 0.06);
    }

    .status-dot {
      width: 12px;
      height: 12px;
      border-radius: 50%;
      background: var(--danger);
      box-shadow: 0 0 0 rgba(255, 107, 107, 0.5);
      transition: background 0.2s ease, box-shadow 0.2s ease;
    }

    .status-dot.connected {
      background: var(--ok);
      box-shadow: 0 0 14px rgba(46, 230, 166, 0.7);
    }

    .status-text {
      font-size: 0.95rem;
      color: var(--text);
    }

    .mode-panel {
      margin-top: 20px;
    }

    .section-label {
      margin: 0 0 12px;
      color: var(--muted);
      text-align: center;
      font-size: 0.88rem;
      letter-spacing: 0.08em;
    }

    .mode-buttons {
      display: grid;
      grid-template-columns: repeat(2, minmax(0, 1fr));
      gap: 12px;
    }

    .mode-btn {
      min-height: 54px;
      border: 1px solid rgba(115, 185, 255, 0.18);
      border-radius: 18px;
      background:
        linear-gradient(180deg, rgba(77, 163, 255, 0.2), rgba(77, 163, 255, 0.08)),
        rgba(10, 17, 32, 0.88);
      color: var(--text);
      box-shadow: inset 0 0 0 1px rgba(255, 255, 255, 0.03), var(--shadow);
      font-size: 0.96rem;
      font-weight: 700;
      cursor: pointer;
      transition: transform 0.08s ease, border-color 0.16s ease, background 0.16s ease;
    }

    .mode-btn:active,
    .mode-btn.active {
      transform: scale(0.98);
      border-color: rgba(46, 230, 166, 0.7);
      background:
        linear-gradient(180deg, rgba(46, 230, 166, 0.34), rgba(77, 163, 255, 0.16)),
        rgba(10, 17, 32, 0.96);
    }

    .pad {
      margin: 28px auto 0;
      width: calc(var(--button-size) * 3 + var(--gap) * 2);
      display: grid;
      grid-template-columns: repeat(3, var(--button-size));
      grid-template-rows: repeat(3, var(--button-size));
      gap: var(--gap);
      justify-content: center;
      align-content: center;
      touch-action: none;
    }

    .control-btn {
      border: 1px solid rgba(115, 185, 255, 0.2);
      border-radius: 24px;
      background:
        linear-gradient(180deg, rgba(77, 163, 255, 0.24), rgba(77, 163, 255, 0.08)),
        rgba(10, 17, 32, 0.85);
      color: var(--text);
      box-shadow: inset 0 0 0 1px rgba(255, 255, 255, 0.03), var(--shadow);
      font-size: 1rem;
      font-weight: 700;
      letter-spacing: 0.08em;
      display: flex;
      align-items: center;
      justify-content: center;
      cursor: pointer;
      transition: transform 0.08s ease, border-color 0.16s ease, background 0.16s ease;
    }

    .control-btn:active,
    .control-btn.active {
      transform: scale(0.96);
      border-color: rgba(115, 185, 255, 0.8);
      background:
        linear-gradient(180deg, rgba(77, 163, 255, 0.45), rgba(77, 163, 255, 0.18)),
        rgba(10, 17, 32, 0.95);
    }

    .control-btn.stop {
      background:
        linear-gradient(180deg, rgba(255, 107, 107, 0.38), rgba(255, 107, 107, 0.14)),
        rgba(24, 10, 14, 0.92);
      border-color: rgba(255, 107, 107, 0.24);
    }

    .hint {
      margin: 24px 0 0;
      color: var(--muted);
      text-align: center;
      font-size: 0.88rem;
      line-height: 1.5;
    }

    .up { grid-column: 2; grid-row: 1; }
    .left { grid-column: 1; grid-row: 2; }
    .stop { grid-column: 2; grid-row: 2; }
    .right { grid-column: 3; grid-row: 2; }
    .down { grid-column: 2; grid-row: 3; }
  </style>
</head>
<body>
  <main class="panel">
    <h1 class="title">网球拾取机器人</h1>
    <p class="subtitle">WebSocket 实时控制中枢</p>

    <section class="status" aria-label="连接状态">
      <span id="statusDot" class="status-dot"></span>
      <span id="statusText" class="status-text">正在连接 ESP32...</span>
    </section>

    <section class="mode-panel" aria-label="模式切换">
      <p class="section-label">工作模式</p>
      <div class="mode-buttons">
        <button id="manualModeBtn" class="mode-btn active" data-mode="M">手动控制模式</button>
        <button id="pickupModeBtn" class="mode-btn" data-mode="P">捡球模式</button>
      </div>
    </section>

    <section class="pad" aria-label="方向控制">
      <button class="control-btn up" data-command="F">前进</button>
      <button class="control-btn left" data-command="L">左转</button>
      <button class="control-btn stop" data-command="S">停止</button>
      <button class="control-btn right" data-command="R">右转</button>
      <button class="control-btn down" data-command="B">后退</button>
    </section>

    <p class="hint">按下方向键立即发送控制指令，松开后自动发送停止指令。</p>
  </main>

  <script>
    (() => {
      const statusDot = document.getElementById("statusDot");
      const statusText = document.getElementById("statusText");
      const buttons = document.querySelectorAll(".control-btn");
      const modeButtons = document.querySelectorAll(".mode-btn");
      const wsUrl = location.hostname ? `ws://${location.host}/ws` : "ws://192.168.4.1/ws";
      const repeatIntervalMs = 180;
      const reconnectDelayMs = 1500;

      let socket = null;
      let reconnectTimer = null;
      let holdTimer = null;
      let activeCommand = null;
      let currentMode = "M";
      let touchActive = false;

      function updateConnectionState(connected, text) {
        statusDot.classList.toggle("connected", connected);
        statusText.textContent = text;
      }

      function clearHoldTimer() {
        if (holdTimer) {
          clearInterval(holdTimer);
          holdTimer = null;
        }
      }

      function isSocketReady() {
        return socket && socket.readyState === WebSocket.OPEN;
      }

      function sendCommand(command) {
        if (!isSocketReady()) {
          return false;
        }
        socket.send(command);
        return true;
      }

      function setActiveButton(command, pressed) {
        buttons.forEach((button) => {
          button.classList.toggle("active", pressed && button.dataset.command === command);
        });
      }

      function setActiveMode(mode) {
        currentMode = mode;
        modeButtons.forEach((button) => {
          button.classList.toggle("active", button.dataset.mode === mode);
        });
      }

      function stopCurrentCommand(sendStop = true) {
        if (!activeCommand) {
          return;
        }
        clearHoldTimer();
        setActiveButton(activeCommand, false);
        activeCommand = null;
        if (sendStop) {
          sendCommand("S");
        }
      }

      function startCommand(command) {
        if (activeCommand === command) {
          return;
        }

        if (activeCommand && activeCommand !== "S") {
          stopCurrentCommand(false);
        }

        activeCommand = command;
        setActiveButton(command, true);
        sendCommand(command);
        clearHoldTimer();

        holdTimer = setInterval(() => {
          sendCommand(command);
        }, repeatIntervalMs);
      }

      function switchMode(mode) {
        if (currentMode === mode) {
          return;
        }

        stopCurrentCommand(true);
        setActiveMode(mode);
        sendCommand(mode);
      }

      function connectWebSocket() {
        if (socket && (socket.readyState === WebSocket.OPEN || socket.readyState === WebSocket.CONNECTING)) {
          return;
        }

        updateConnectionState(false, "正在连接 ESP32...");
        socket = new WebSocket(wsUrl);

        socket.addEventListener("open", () => {
          if (reconnectTimer) {
            clearTimeout(reconnectTimer);
            reconnectTimer = null;
          }
          updateConnectionState(true, "已连接 ESP32");
          sendCommand(currentMode);
        });

        socket.addEventListener("close", () => {
          updateConnectionState(false, "连接断开，正在重连...");
          stopCurrentCommand(false);
          if (!reconnectTimer) {
            reconnectTimer = setTimeout(() => {
              reconnectTimer = null;
              connectWebSocket();
            }, reconnectDelayMs);
          }
        });

        socket.addEventListener("error", () => {
          updateConnectionState(false, "通信异常，正在重连...");
          socket.close();
        });
      }

      function bindPressEvents(button) {
        const command = button.dataset.command;

        const handlePress = (event) => {
          event.preventDefault();
          if (event.type === "touchstart") {
            touchActive = true;
          } else if (touchActive) {
            return;
          }
          startCommand(command);
        };

        const handleRelease = (event) => {
          event.preventDefault();
          if (event.type === "touchend" || event.type === "touchcancel") {
            touchActive = false;
          }
          stopCurrentCommand(true);
        };

        button.addEventListener("touchstart", handlePress, { passive: false });
        button.addEventListener("mousedown", handlePress);
        button.addEventListener("touchend", handleRelease, { passive: false });
        button.addEventListener("touchcancel", handleRelease, { passive: false });
        button.addEventListener("mouseup", handleRelease);
        button.addEventListener("mouseleave", () => {
          if (activeCommand === command) {
            stopCurrentCommand(true);
          }
        });
      }

      function bindModeEvents(button) {
        const mode = button.dataset.mode;

        const handleSwitch = (event) => {
          event.preventDefault();
          if (event.type === "touchstart") {
            touchActive = true;
          } else if (touchActive) {
            return;
          }
          switchMode(mode);
        };

        button.addEventListener("mousedown", handleSwitch);
        button.addEventListener("touchstart", handleSwitch, { passive: false });
      }

      buttons.forEach(bindPressEvents);
      modeButtons.forEach(bindModeEvents);
      setActiveMode(currentMode);

      document.addEventListener("mouseup", () => stopCurrentCommand(true));
      document.addEventListener("touchend", () => {
        touchActive = false;
        stopCurrentCommand(true);
      }, { passive: true });
      document.addEventListener("touchcancel", () => {
        touchActive = false;
        stopCurrentCommand(true);
      }, { passive: true });
      window.addEventListener("beforeunload", () => {
        stopCurrentCommand(true);
        if (socket) {
          socket.close();
        }
      });

      connectWebSocket();
    })();
  </script>
</body>
</html>
)rawliteral";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
bool ledState = false;
uint32_t lastHeartbeatMs = 0;

char normalizeCommand(char command) {
  if (command >= 'a' && command <= 'z') {
    command = static_cast<char>(command - 'a' + 'A');
  }
  return command;
}

uint8_t commandToCode(char command) {
  switch (normalizeCommand(command)) {
    case 'F': return 0x01;
    case 'B': return 0x02;
    case 'L': return 0x03;
    case 'R': return 0x04;
    case 'S': return 0x05;
    case 'M': return kManualModeCode;
    case 'P': return kPickupModeCode;
    default: return 0x00;
  }
}

bool forwardCommandToSerial(char command) {
  const uint8_t commandCode = commandToCode(command);
  if (commandCode == 0x00) {
    return false;
  }

  const uint8_t packet[3] = {kPacketHeader, commandCode, kPacketTail};
  Serial2.write(packet, sizeof(packet));
  Serial.printf("WS指令: %c -> 串口包: [0x%02X 0x%02X 0x%02X]\n",
                normalizeCommand(command), packet[0], packet[1], packet[2]);
  return true;
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  auto *frameInfo = reinterpret_cast<AwsFrameInfo *>(arg);
  if (!frameInfo || !frameInfo->final || frameInfo->index != 0 || frameInfo->opcode != WS_TEXT || len == 0) {
    return;
  }

  const char command = normalizeCommand(static_cast<char>(data[0]));
  if (!forwardCommandToSerial(command)) {
    Serial.printf("收到未知指令: %c\n", command);
  }
}

void onWebSocketEvent(AsyncWebSocket *serverInstance,
                      AsyncWebSocketClient *client,
                      AwsEventType type,
                      void *arg,
                      uint8_t *data,
                      size_t len) {
  (void)serverInstance;

  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket 客户端已连接: #%u, IP=%s\n",
                    client->id(), client->remoteIP().toString().c_str());
      client->text("CONNECTED");
      break;

    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket 客户端已断开: #%u\n", client->id());
      break;

    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;

    case WS_EVT_PONG:
    case WS_EVT_ERROR:
    default:
      break;
  }
}

void setupAccessPoint() {
  WiFi.mode(WIFI_AP);
  const bool apStarted = WiFi.softAP(kApSsid, kApPassword);

  if (apStarted) {
    Serial.println("AP 热点已开启");
    Serial.printf("SSID: %s\n", kApSsid);
    Serial.printf("PASS: %s\n", kApPassword);
    Serial.printf("管理地址: http://%s/\n", WiFi.softAPIP().toString().c_str());
  } else {
    Serial.println("AP 热点启动失败");
  }
}

void setupWebServer() {
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html; charset=utf-8", kControlPage);
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->redirect("/");
  });

  server.begin();
  Serial.println("异步 Web 服务器已启动");
}

}  // namespace

void setup() {
  Serial.begin(kDebugBaudRate);
  Serial2.begin(kSerial2BaudRate, SERIAL_8N1, kSerial2RxPin, kSerial2TxPin);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.println();
  Serial.println("自动网球拾取机器人通信中枢启动中...");
  Serial.printf("Serial2 已初始化: RX=%u, TX=%u, Baud=%lu\n",
                kSerial2RxPin, kSerial2TxPin, static_cast<unsigned long>(kSerial2BaudRate));

  setupAccessPoint();
  setupWebServer();
}

void loop() {
  const uint32_t now = millis();
  if (now - lastHeartbeatMs >= kHeartbeatIntervalMs) {
    lastHeartbeatMs = now;
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
  }

  ws.cleanupClients();
}
