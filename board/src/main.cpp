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