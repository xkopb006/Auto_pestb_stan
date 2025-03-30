/* --- IMPORT KNIHOVEN --------------------------------------- */
#include <Wire.h>               // komunikace I2C knihovna
#include <DHT.h>                // ASIAR DH11 senzor teplota/vlhkost vzduchu
#include <Adafruit_BMP280.h>    // BMP280 senzor teplota/tlak vzduchu
#include <OneWire.h>            // komunikace např. DS18B20
#include <DallasTemperature.h>  // práce se DS18B20
#include <SoftwareSerial.h>     // simulace sériové komunikace
#include <SPI.h>
#include <WiFi.h>               // WiFi komunikace
#include <PubSubClient.h>       // MQTT protokol
#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// import konfigurace pinů, wifi a mqtt
#include "pinout.h"
#include "wifi_mqtt_config.h"

// import souborů pro řízení komponentů
#include "led_control.h"
#include "pump_control.h"
#include "outfan_control.h"
#include "infan_control.h"
#include "ph_control.h"
#include "tds_control.h"


/* --- INICIALIZACE SENZORŮ --------------------------------------- */
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;
OneWire oneWire(DS18B20);
DallasTemperature ds18b20(&oneWire);
SoftwareSerial pzemSerial(PZEM_RX, PZEM_TX);


/* --- KONFIGURACE NTP ----------------------------------------------- */
WiFiUDP ntp_UDP;
NTPClient timeClient(ntp_UDP, "pool.ntp.org", 3600, 60000);


/* --- FUNKCE PRO WIFI A MQTT --------------------------------------- */
void setup_wifi() {
    delay(10);
    Serial.println("Připojování k WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n---- WiFi připojeno --------------------------------------------------------");
}

unsigned long last_reconnect_attempt = 0;
const long reconnect_interval = 5000;
void reconnect() {
    if (!client.connected() && (millis() - last_reconnect_attempt > reconnect_interval)) {
        last_reconnect_attempt = millis();
        Serial.println("Připojování k MQTT...");
        Serial.println(mqtt_server);
        if (client.connect("ArduinoClient", broker_user, broker_password)) {
            Serial.println("---- MQTT připojeno ------------------------------------------------------------");
            client.subscribe("control/#");
        } else {
            Serial.print("Chyba, rc=");
            Serial.print(client.state());
            Serial.println(" - pokus o opětovné připojení za 5 sekund");
        }
    }
}


/* --- FUNKCE PRO ČTENÍ A LOGGOVÁNÍ DAT ZE SENZORŮ --------------------------------------- */
void log_sensor_data() {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    float temperaturebmp = bmp.readTemperature();
    float pressure = bmp.readPressure() / 100.0F;
    ds18b20.requestTemperatures();
    float water_temperature = ds18b20.getTempCByIndex(0);
    float ph_value = getPHValue();
    int tds_value = analogRead(TDS_SENSOR);

    controlOutfanBasedOnConditions(temperature, humidity);

    Serial.println("\nAktualizace senzorů...");
    Serial.print("Teplota vzduchu(dht): ");
    Serial.print(temperature);
    Serial.println(" °C");
    Serial.print("Vlhkost vzduchu: ");
    Serial.print(humidity);
    Serial.println(" %");
    Serial.print("Teplota vzduchu(bmp280): ");
    Serial.print(temperaturebmp);
    Serial.println(" °C");
    Serial.print("Tlak vzduchu: ");
    Serial.print(pressure);
    Serial.println(" hPa");
    Serial.print("Teplota vody: ");
    Serial.print(water_temperature);
    Serial.println(" °C");
    Serial.print("Hodnota pH: ");
    Serial.println(ph_value);
    Serial.print("Hodnota TDS: ");
    Serial.println(tds_value);

    char buffer[50];
    snprintf(buffer, 50, "%.2f", temperature);
    client.publish("sensor/temperature", buffer);
    snprintf(buffer, 50, "%.2f", humidity);
    client.publish("sensor/humidity", buffer);
    snprintf(buffer, 50, "%.2f", temperaturebmp);
    client.publish("sensor/temperaturebmp", buffer);
    snprintf(buffer, 50, "%.2f", pressure);
    client.publish("sensor/pressure", buffer);
    snprintf(buffer, 50, "%.2f", water_temperature);
    client.publish("sensor/water_temperature", buffer);
    snprintf(buffer, 50, "%.2f", ph_value);
    client.publish("sensor/ph", buffer);
    snprintf(buffer, 50, "%d", tds_value);
    client.publish("sensor/tds", buffer);
}


/* --- CALLBACK FUNKCE PRO ZPRACOVÁNÍ MQTT ZPRÁVA A REAKCÍ NA NĚ --------------------------------------- */
void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.print("Přijato: ");
    Serial.print(topic);
    Serial.print(" - ");
    Serial.println(message);

    if (strcmp(topic, "control/led") == 0) {
        controlLEDManually(message);
    }
    else if (strcmp(topic, "control/led/schedule") == 0) {
        processScheduleMessage(message);
    }
    else if (strcmp(topic, "control/pump") == 0) {
        controlPumpManually(message);
    }
    else if (strcmp(topic, "control/outfan") == 0) {
        controlOutfanManually(message);
    }
    else if (strcmp(topic, "control/outfan/settings") == 0) {
        configureOutfanSettings(message);
    }
    else if (strcmp(topic, "control/infan") == 0) {
        controlInfanManually(message);
    }
    else if (strcmp(topic, "control/infan/interval") == 0) {
        processInfanInterval(message);
    }
    else if (strcmp(topic, "control/ph") == 0) {
        controlPHByThresholds(message);
    }
    else if(strcmp(topic, "control/tds") == 0) {
        setTdsMinThreshold(message);
    }
}

// SETUP PŘIPOJENÍ K WIFI A MQTT SERVERU
void initNetwork() {
    setup_wifi();
    client.setServer(mqtt_server, 1884);
    client.setCallback(callback);
}

// SETUP SENZORŮ
void initSensors(){
    Wire.begin();                           // I2C sběrnice
    dht.begin();                            // DHT11 senzor
    bmp.begin(0x76);                        // BMP280 senzor
    ds18b20.begin();                        // DS18B20 senzor
    pzemSerial.begin(9600);                 // sériový port pro PZEM-004T
    timeClient.begin();                     // čas
}

// SETUP RELÉ
void initRelays(){
    pinMode(RELAY_LED, OUTPUT);
    digitalWrite(RELAY_LED, HIGH);
    client.publish("control/led", "VYPNUTO", true);

    pinMode(RELAY_PUMP, OUTPUT);
    digitalWrite(RELAY_PUMP, HIGH);
    client.publish("control/pump", "VYPNUTO", true);

    pinMode(RELAY_OUTFAN, OUTPUT);
    digitalWrite(RELAY_OUTFAN, HIGH);
    client.publish("control/outfan", "VYPNUTO", true);

    pinMode(RELAY_INFAN, OUTPUT);
    digitalWrite(RELAY_INFAN, HIGH);
    client.publish("control/infan", "VYPNUTO", true);
}

// SETUP H-MŮSTKŮ
void initHBridges(){
    pinMode(H_BRIDGE_PH_PLUS, OUTPUT);
    pinMode(H_BRIDGE_PH_MINUS, OUTPUT);
    digitalWrite(H_BRIDGE_PH_PLUS, LOW);
    digitalWrite(H_BRIDGE_PH_MINUS, LOW);
    client.publish("control/ph", "VYPNUTO", true);

    pinMode(H_BRIDGE_TDS_A, OUTPUT);
    pinMode(H_BRIDGE_TDS_B, OUTPUT);
    digitalWrite(H_BRIDGE_TDS_A, LOW);
    digitalWrite(H_BRIDGE_TDS_B, LOW);
    client.publish("control/tds", "VYPNUTO", true);
}

// INICIALIZACE ČASU
void updateTime() {
    timeClient.update();
    setTime(timeClient.getEpochTime());
}


// *** SETUP *************************** //
void setup() {
    Serial.begin(115200);

    initNetwork();
    initSensors();
    initRelays();
    initHBridges();
}


unsigned long previous_fast_millis = 0;
unsigned long previous_slow_millis = 0;
const unsigned long interval_fast = 5000;
const unsigned long interval_slow = 20000;
// *** LOOP *************************** //
void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    updateTime();

    unsigned long current_millis = millis();

    if (current_millis - previous_fast_millis >= interval_fast) {
        previous_fast_millis = current_millis;
        log_sensor_data();
        processTDSThreshold();
        // LED INTERVAL
        controlLEDBySchedule();
        // INFAN INTEVRAL
        controlInfanBasedOnSettings();
    }
    // pH MEZE
    processPHThresholds();
    // TDS MINIMUM
    processTDSThreshold();
}}