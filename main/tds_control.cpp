#include "tds_control.h"
#include "pinout.h"
#include "wifi_mqtt_config.h"

float tds_min_threshold = 0.0;
unsigned long last_TDS_regulation = 0;
const unsigned long TDS_interval = 120000;

bool tds_pump_active = false;
unsigned long tds_pump_start_time = 0;

// zpracování zprávy o nastavení minimální TDS hodnoty pro automatické řízení
void setTdsMinThreshold(String message) {
    tds_min_threshold = message.toFloat();
    Serial.print("TDS min: ");
    Serial.println(tds_min_threshold);
}

// automatická regulace živin podle minimální TDS hodnoty
void processTDSThreshold() {
    if (tds_pump_active) {
        if (millis() - tds_pump_start_time >= 5000) {
            digitalWrite(H_BRIDGE_TDS_A, LOW);
            digitalWrite(H_BRIDGE_TDS_B, LOW);
            tds_pump_active = false;
            last_TDS_regulation = millis();
            Serial.println("konec zvyšení TDS");
        }
        return;
    }

    if (millis() - last_TDS_regulation < TDS_interval) {
        return;
    }

    int tds_value = analogRead(TDS_SENSOR);
    Serial.print("aktuální TDS: ");
    Serial.println(tds_value);

    // TDS nižší než minimální hodnota - aktivuj pumpy
    if (tds_value < tds_min_threshold) {
        Serial.println("TDS < min --> + hnojiva A a B");
        digitalWrite(H_BRIDGE_TDS_A, HIGH);
        digitalWrite(H_BRIDGE_TDS_B, HIGH);
        tds_pump_active = true;
        tds_pump_start_time = millis();
    }
    else {
        digitalWrite(H_BRIDGE_TDS_A, LOW);
        digitalWrite(H_BRIDGE_TDS_B, LOW);
    }
}
