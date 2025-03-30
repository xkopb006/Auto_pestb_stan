#include "wifi_mqtt_config.h"

// připojení k wifi
const char* ssid = "Koptik";
const char* password = "vacice420";

// připojení k mqtt brokeru
const char* mqtt_server = "10.0.1.41";
const char* broker_user = "mqttuser";
const char* broker_password = "12345";

WiFiClient esp_client;
PubSubClient client(esp_client);
