#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "secrets.h"

#define DHT_PIN 15
#define LED_PIN 2

const char* API_URL = "http://iot-yahira.chilecentral.cloudapp.azure.com/readings";
const char* MQTT_HOST = "135.232.203.144";
const int MQTT_PORT = 1883;
const char* T_SETTINGS = "ecse3038/yahira/fan/settings";
const char* T_STATE = "ecse3038/yahira/fan/state";
const char* T_STATUS = "ecse3038/yahira/status";

DHT dht(DHT_PIN, DHT22);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

String mode = "off";
float maxHumidity = 100;
float humidity = 0;
bool fanOn = false;
unsigned long lastReading = 0;

void connectWifi() {
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) delay(250);
}

void connectMqtt() {
  while (!mqtt.connected()) {
    String id = "fan-" + WiFi.macAddress();
    if (mqtt.connect(id.c_str(), T_STATUS, 0, true, "offline")) {
      mqtt.publish(T_STATUS, "online", true);
      mqtt.subscribe(T_SETTINGS);
    } else {
      delay(1000);
    }
  }
}

void updateFan() {
  bool shouldBeOn = false;
  if (mode == "on") shouldBeOn = true;
  else if (mode == "auto") shouldBeOn = humidity > maxHumidity;
  digitalWrite(LED_PIN, shouldBeOn ? HIGH : LOW);
  if (shouldBeOn != fanOn) {
    fanOn = shouldBeOn;
    mqtt.publish(T_STATE, fanOn ? "on" : "off", true);
  }
}

void onMessage(char* topic, byte* payload, unsigned int length) {
  JsonDocument doc;
  if (deserializeJson(doc, payload, length)) return;
  if (!doc["mode"].is<const char*>() || !doc["max_humidity"].is<float>()) return;
  String newMode = doc["mode"].as<String>();
  if (newMode != "auto" && newMode != "on" && newMode != "off") return;
  mode = newMode;
  maxHumidity = doc["max_humidity"].as<float>();
  updateFan();
}

void sendReading() {
  float h = dht.readHumidity();
  if (isnan(h)) return;
  humidity = h;
  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Key", DEVICE_KEY);
  String body = "{\"mac\":\"" + WiFi.macAddress() + "\",\"humidity\":" + String(humidity) + "}";
  int code = http.POST(body);
  Serial.println("POST /readings -> " + String(code));
  http.end();
  updateFan();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  dht.begin();
  connectWifi();
  Serial.println("MAC: " + WiFi.macAddress());
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
  connectMqtt();
}

void loop() {
  if (!mqtt.connected()) connectMqtt();
  mqtt.loop();
  if (millis() - lastReading >= 10000 || lastReading == 0) {
    lastReading = millis();
    sendReading();
  }
}