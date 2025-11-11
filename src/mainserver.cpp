#include "mainserver.h"
#include "global.h"
#include <WiFiS3.h>
#include "coreiot.h"


const char *ap_ssid = "UNO_R4_AP";
const char *ap_pass = "12345678";
WiFiServer server(80);
bool need_revert_AP = false;
bool isAPMode = true;
bool connecting = false;
unsigned long connect_start_ms = 0;
String wifi_ssid = "";
String wifi_password = "";

// HTML Main Page
String mainPage()
{
  String page = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
      <meta charset="utf-8">
      <title>UNO R4 WiFi Dashboard</title>
      <style>
          body {
              background-color: #f5f7fa;
              color: #222;
              font-family: "Segoe UI", Arial, sans-serif;
              text-align: center;
              margin: 0;
              padding: 0;
          }
          header {
              background: linear-gradient(90deg, #0078d7, #00a8ff);
              color: white;
              padding: 20px 0;
              font-size: 22px;
              font-weight: bold;
              box-shadow: 0 2px 5px rgba(0,0,0,0.2);
          }
          .card {
              display: inline-block;
              background: white;
              border-radius: 12px;
              box-shadow: 0 4px 10px rgba(0,0,0,0.1);
              margin: 20px;
              padding: 20px;
              width: 280px;
              transition: 0.3s;
          }
          .card:hover { transform: translateY(-4px); }
          .sensor-value {
              font-size: 40px;
              font-weight: bold;
              color: #0078d7;
          }
          .button {
              padding: 12px 25px;
              margin: 8px;
              border: none;
              border-radius: 8px;
              font-size: 15px;
              cursor: pointer;
              color: white;
              transition: 0.2s;
          }
          .on { background-color: #28a745; }
          .off { background-color: #dc3545; }
          .on:hover { background-color: #218838; }
          .off:hover { background-color: #c82333; }
          footer {
              margin-top: 30px;
              font-size: 13px;
              color: #666;
          }
          a {
              color: #0078d7;
              text-decoration: none;
          }
          a:hover {
              text-decoration: underline;
          }
      </style>
  </head>
  <body>
      <header>🌤️ UNO R4 WiFi DHT Dashboard</header>

      <div class="card">
          <h2>🌡️ Temperature</h2>
          <div class="sensor-value" id="temp">--</div>
          <p>°C</p>
      </div>

      <div class="card">
          <h2>💧 Humidity</h2>
          <div class="sensor-value" id="humi">--</div>
          <p>%</p>
      </div>

      <div class="card">
          <h2>💡 LED Controls</h2>
          <button class="button on" onclick="fetch('/led1?state=on')">LED 1 ON</button>
          <button class="button off" onclick="fetch('/led1?state=off')">LED 1 OFF</button><br>
          <button class="button on" onclick="fetch('/led2?state=on')">LED 2 ON</button>
          <button class="button off" onclick="fetch('/led2?state=off')">LED 2 OFF</button>
      </div>

      <p><a href="/settings">⚙️ Wi-Fi Settings</a></p>

      <footer>UNO R4 WiFi © 2025 | CoreIoT Demo</footer>

      <script>
        async function updateSensor() {
          try {
            const res = await fetch('/sensors');
            const data = await res.json();
            document.getElementById('temp').innerText = data.temperature.toFixed(1);
            document.getElementById('humi').innerText = data.humidity.toFixed(1);
          } catch (e) { console.log('Fetch error:', e); }
        }
        setInterval(updateSensor, 2000);
        updateSensor();
      </script>
  </body>
  </html>
  )rawliteral";
  return page;
}

// HTML WiFi Settings Page
String settingsPage()
{
  String page = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
      <meta charset="utf-8">
      <title>WiFi Settings</title>
      <style>
          body {
              background-color: #f5f7fa;
              color: #222;
              font-family: "Segoe UI", Arial, sans-serif;
              text-align: center;
              margin: 0;
              padding: 0;
          }
          header {
              background: linear-gradient(90deg, #0078d7, #00a8ff);
              color: white;
              padding: 20px 0;
              font-size: 22px;
              font-weight: bold;
              box-shadow: 0 2px 5px rgba(0,0,0,0.2);
          }
          .container {
              margin-top: 50px;
              background: white;
              display: inline-block;
              border-radius: 12px;
              box-shadow: 0 4px 10px rgba(0,0,0,0.1);
              padding: 30px;
          }
          input, button {
              padding: 12px;
              margin: 10px;
              width: 240px;
              border: 1px solid #ccc;
              border-radius: 8px;
              font-size: 15px;
          }
          button {
              background-color: #0078d7;
              color: white;
              border: none;
              cursor: pointer;
              transition: 0.2s;
          }
          button:hover {
              background-color: #005fa3;
          }
          a {
              color: #0078d7;
              text-decoration: none;
              display: block;
              margin-top: 15px;
          }
      </style>
  </head>
  <body>
      <header>⚙️ Wi-Fi Configuration</header>

      <div class="container">
          <form action="/connect" method="GET">
              <input type="text" name="ssid" placeholder="WiFi SSID" required><br>
              <input type="password" name="pass" placeholder="Password"><br>
              <button type="submit">Connect</button>
          </form>
          <a href="/">← Back to Dashboard</a>
      </div>
  </body>
  </html>
  )rawliteral";
  return page;
}
// ====================== GET PARAM PARSER ======================
String getParamValue(const String &req, const String &key)
{
  int idx = req.indexOf(key + "=");
  if (idx == -1) return "";
  int start = idx + key.length() + 1;
  int end = req.indexOf('&', start);
  if (end == -1) end = req.indexOf(' ', start);
  return req.substring(start, end);
}

void handleConnect(const String &req)
{
  wifi_ssid = getParamValue(req, "ssid");
  wifi_password = getParamValue(req, "pass");
  Serial.print("WiFi SSID: "); Serial.println(wifi_ssid);
  Serial.print("WiFi PASS: "); Serial.println(wifi_password);

  connecting = true;
  connect_start_ms = millis();
}

int lastWiFiStatus = WL_DISCONNECTED;

// --------------------------------------------------
// Hàm revert về AP mode khi mất kết nối STA
// --------------------------------------------------

// --------------------------------------------------
void startAP()
{
  WiFi.disconnect();
  vTaskDelay(pdMS_TO_TICKS(500));

  if (WiFi.beginAP(ap_ssid, ap_pass) == WL_AP_LISTENING)
  {
    Serial.println("[WiFi] Access Point mode started successfully!");
    Serial.print("SSID: ");
    Serial.println(ap_ssid);
    Serial.print("Password: ");
    Serial.println(ap_pass);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    server.begin();
  }
  else
  {
    Serial.println("[WiFi] Failed to start Access Point mode!");
  }
}

void revertToAP()
{
  Serial.println("[WiFi] Connection lost → Reverting to AP mode...");
  WiFi.disconnect();
  vTaskDelay(pdMS_TO_TICKS(1000));
  startAP();
  isAPMode = true;
}



bool startSTA(const char *ssid, const char *pass)
{
  Serial.print("[WiFi] Trying to connect to WiFi: ");
  Serial.println(ssid);

  WiFi.disconnect();
  vTaskDelay(pdMS_TO_TICKS(500));
  WiFi.begin(ssid, pass);

  unsigned long start = millis();
  const unsigned long timeout = 10000; // 10 giây

  while (WiFi.status() != WL_CONNECTED && millis() - start < timeout)
  {
    Serial.print(".");
    vTaskDelay(pdMS_TO_TICKS(500));
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\n[WiFi] Connected successfully!");
    Serial.print("STA IP: ");
    Serial.println(WiFi.localIP());
    isAPMode = false;

    // ✅ Chạy task CoreIoT sau khi kết nối thành công
    xTaskCreate(coreiot_task, "COREIOT", 512, NULL, 2, NULL);
    return true;
  }
  else
  {
    Serial.println("\n[WiFi] Connection failed, returning to AP mode...");
    revertToAP();
    return false;
  }
}


// --------------------------------------------------
// --------------------------------------------------
void webserver_task(void *pvParameters)
{
  Serial.println("[HTTP] webserver_task started");
  startAP();

  SensorData latestData = {0, 0};

  while (1)
  {
    static int lostCount = 0;
    int currentStatus = WiFi.status();

    if (!isAPMode) {
      if (currentStatus != WL_CONNECTED || WiFi.RSSI() == 0 || WiFi.RSSI() == -127) {
        lostCount++;
        if (lostCount >= 5) { // mất 5 lần liên tiếp (~2.5s)
          Serial.println("[WiFi] Lost connection confirmed, reverting to AP mode...");
          revertToAP();
          lostCount = 0;
        }
      } else {
        lostCount = 0; // reset nếu kết nối ổn lại
      }
    }

    vTaskDelay(pdMS_TO_TICKS(500)); // kiểm tra mỗi 0.5s
    WiFiClient client = server.available();
    if (!client)
    {
      vTaskDelay(pdMS_TO_TICKS(50));
      continue;
    }

    Serial.println("[HTTP] Client connected");
    String req = client.readStringUntil('\r');
    client.flush();

    // ---------- /connect ----------
    if (req.startsWith("GET /connect"))
    {
      int ssidIndex = req.indexOf("ssid=");
      int passIndex = req.indexOf("&pass=");
      if (ssidIndex > 0 && passIndex > 0)
      {
        wifi_ssid = req.substring(ssidIndex + 5, passIndex);
        wifi_password = req.substring(passIndex + 6, req.indexOf(" ", passIndex));

        wifi_ssid.replace("%20", " ");
        wifi_password.replace("%20", " ");

        Serial.println("[HTTP] User input:");
        Serial.print("SSID: ");
        Serial.println(wifi_ssid);
        Serial.print("PASS: ");
        Serial.println(wifi_password);

        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("Connection: close");
        client.println();
        client.println("<html><body><h3>Trying to connect...</h3><p>Check Serial Monitor for status.</p></body></html>");
        client.stop();

        startSTA(wifi_ssid.c_str(), wifi_password.c_str());
        continue;
      }
    }

    // ---------- /settings ----------
    if (req.indexOf("GET /settings") != -1)
    {
      Serial.println("[HTTP] Serving settings page...");
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println();
      client.print(settingsPage());
      vTaskDelay(pdMS_TO_TICKS(10));
      client.stop();
      continue;
    }

    // ---------- /sensors ----------
    if (req.indexOf("GET /sensors") != -1)
    {
      if (xQueuePeek(qTempHumi, &latestData, 0) != pdTRUE)
      {
        Serial.println("[HTTP] No sensor data yet");
        latestData.temperature = -1;
        latestData.humidity = -1;
      }

      String json = "{\"temperature\":";
      json += String(latestData.temperature, 1);
      json += ",\"humidity\":";
      json += String(latestData.humidity, 1);
      json += "}";

      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.print(json);
      client.stop();
      continue;
    }

    // ---------- LED Control ----------
    if (req.indexOf("/led1?state=on") != -1)
      digitalWrite(5, HIGH);
    else if (req.indexOf("/led1?state=off") != -1)
      digitalWrite(5, LOW);
    else if (req.indexOf("/led2?state=on") != -1)
      digitalWrite(4, HIGH);
    else if (req.indexOf("/led2?state=off") != -1)
      digitalWrite(4, LOW);

    // ---------- Default: main page ----------
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.print(mainPage());
    vTaskDelay(pdMS_TO_TICKS(150));
    client.stop();
  }
}