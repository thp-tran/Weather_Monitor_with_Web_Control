#include "coreiot.h"

// ----------- CONFIGURE THESE -----------
const char* coreIOT_Server = "app.coreiot.io";
const char* coreIOT_Token  = "2dh8vkxq8os6718zenr9";  
const int   mqttPort        = 1883;
// --------------------------------------

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void reconnect_coreiot() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to CoreIoT...");
    // username = AccessToken, password = ""
    if (mqttClient.connect("UNO_R4_Client", coreIOT_Token, NULL)) {
      Serial.println(" connected!");
      mqttClient.subscribe("v1/devices/me/rpc/request/+");
      Serial.println("Subscribed to RPC topic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retry in 5s");
      delay(5000);
    }
  }
}

void coreiot_callback(char* topic, byte* payload, unsigned int length) {
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  Serial.print("RPC arrived: ");
  Serial.println(message);

  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, message)) return;

  const char* method = doc["method"];
  if (strcmp(method, "setStateLED") == 0) {
    const char* params = doc["params"];
    if (strcmp(params, "ON") == 0) {
      digitalWrite(3, HIGH);
    } else {
      digitalWrite(3, LOW);
    }
  }
}

void coreiot_task(void *pvParameters) {
  Serial.println("Starting CoreIoT task...");
  mqttClient.setServer(coreIOT_Server, mqttPort);
  mqttClient.setCallback(coreiot_callback);

  SensorData sensorData;

  while (1) {

      if (!mqttClient.connected()) reconnect_coreiot();

      if (xQueuePeek(qTempHumi, &sensorData, 0) != pdTRUE) {
        sensorData.humidity = -1;
        sensorData.temperature = -1;
      }

      // Publish telemetry JSON
      String payload = "{\"temperature\":" + String(sensorData.temperature, 1) +
                       ",\"humidity\":" + String(sensorData.humidity, 1) + "}";
      mqttClient.publish("v1/devices/me/telemetry", payload.c_str());
      Serial.println("Published to CoreIoT: " + payload);

      mqttClient.loop();

    vTaskDelay(pdMS_TO_TICKS(10000));  // 10s mỗi lần gửi
  }
}
