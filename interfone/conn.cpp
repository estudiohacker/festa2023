#include <WiFi.h>
#include "settings.h"
#include "debug.h"
#include "conn.h"
#include "ota.h"
#include "version.h"

WiFiClient wifiClient;

extern String uptime;

long lastMQTTReconnectAttempt = -MQTT_CONNECTION_RETRY;

void setupWiFi(const char* hostname) {
  debugMsg("Starting WiFi Setup...\n");
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(String(hostname));
  WiFi.disconnect();

  debugMsg("Connecting %s to SSID %s", hostname, WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int retries = 0;
  while ((WiFi.status() != WL_CONNECTED) and (retries < WIFI_CONNECTION_RETRIES)) {
    delay(500);
    retries++;
    debugMsg(".");
  }
  debugMsg(" Ok.\n");

  debugMsg("IP address set to %s\n", WiFi.localIP().toString().c_str());
}

Conn::Conn() {
  char chipid[13];
  snprintf(chipid, 13, "%llX", ESP.getEfuseMac());
  this->_id = String(chipid);
  this->_hostname = String(HOSTNAME_PREFIX) + String(this->_id);

  setupWiFi(this->_hostname.c_str());
  setupOTA();

  this->_PubSubClient = new PubSubClient(wifiClient);
  this->_PubSubClient->setServer(MQTT_SERVER, MQTT_PORT);
}

void Conn::connect() {
  if (!this->_PubSubClient->connected()) {
      if (millis() - lastMQTTReconnectAttempt > MQTT_CONNECTION_RETRY) {
        lastMQTTReconnectAttempt = millis();
        debugMsg("Attempting MQTT connection to %s...", MQTT_SERVER);
        if (this->_PubSubClient->connect(this->_hostname.c_str(), MQTT_USER, MQTT_PASSWORD)) {
          debugMsg(" Ok.\n");

          // Once connected, publish announcements...
          notify_topic("version", version, true);
        } else {
          debugMsg(" ERROR: %d. Trying again later.\n", this->_PubSubClient->state());
        }
      }
  }
};

void Conn::loop() {
  loopOTA();
  this->connect();
  this->_PubSubClient->loop();
}

String Conn::fullTopic(String topic) {
  return String(MQTT_TOPIC_PREFIX) + "interfone/" + this->_id + "/" + topic;
}

void Conn::notify_topic(String topic, String payload, bool retained) {
  debugMsg("%s: %s\n", fullTopic(topic).c_str(), payload.c_str());
  digitalWrite(LED_STATUS_PIN, LOW);
  this->_PubSubClient->publish(fullTopic(topic).c_str(), payload.c_str(), retained);
  digitalWrite(LED_STATUS_PIN, HIGH);
}

void Conn::notify_sensor(String sensor, String value) {
  notify_topic("sensor/" + sensor, value, false);
}

void Conn::notify_sensor(String sensor, uint8_t value) {
  notify_sensor(sensor, String(value));
}
