#ifndef WIFI_MQTT_CONFIG_H
#define WIFI_MQTT_CONFIG_H

#include <WiFi.h>
#include <PubSubClient.h>

// --- KONFIGURACE WiFi ---------------------------------------
extern const char* ssid;
extern const char* password;

// --- KONFIGURACE MQTT ---------------------------------------
extern const char* mqtt_server;
extern const char* broker_user;
extern const char* broker_password;

extern WiFiClient esp_client;
extern PubSubClient client;

#endif