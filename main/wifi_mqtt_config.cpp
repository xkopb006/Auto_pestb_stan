#include "wifi_mqtt_config.h"

// připojení k wifi
const char* ssid = "ssid";
const char* password = "password";

// připojení k mqtt brokeru
const char* mqtt_server = "0.0.0.0";
const char* broker_user = "mqttuser";
const char* broker_password = "12345";

WiFiClient esp_client;
PubSubClient client(esp_client);
