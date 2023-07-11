#pragma once

#include <Arduino.h>

#define DEBUG

static const char* HOSTNAME_PREFIX = "interfone";

static const char* WIFI_SSID = "I9";
static const char* WIFI_PASSWORD = "12345678";
static const uint8_t WIFI_CONNECTION_RETRIES = 20;

static const unsigned long MQTT_CONNECTION_RETRY = 60000;
static const char* MQTT_SERVER = "192.168.10.10";
static const uint16_t MQTT_PORT = 1883;
static const char* MQTT_USER = "";
static const char* MQTT_PASSWORD = "";
static const char* MQTT_TOPIC_PREFIX = "";

static const uint8_t LED_STATUS_PIN = 2;
