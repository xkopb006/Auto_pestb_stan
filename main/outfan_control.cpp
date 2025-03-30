#include "outfan_control.h"
#include "pinout.h"
#include "wifi_mqtt_config.h"

bool is_border_active = false;
bool temp_active = false;
bool humidity_active = false;
String last_outfan_state = "VYPNUTO";
float threshold_temperature = NAN;
float threshold_humidity = NAN;

// manuální řížení pomocí ON/OFF checkoboxu
void controlOutfanManually(String message) {
    if (!is_border_active && message != last_outfan_state) {
        digitalWrite(RELAY_OUTFAN, (message == "ZAPNUTO") ? LOW : HIGH);
        last_outfan_state = message;
        Serial.println(message);
    }
}

// automatické řízení vnějšího větráku podle aktuálních hodnot na senzorech a mezních hodnot dle nastavení
void controlOutfanBasedOnConditions(float temperature, float humidity) {
    if (is_border_active) {
        String new_state = "VYPNUTO";
        if ((!isnan(threshold_temperature) && temperature >= threshold_temperature) ||
            (!isnan(threshold_humidity) && humidity >= threshold_humidity)) {
            new_state = "ZAPNUTO";
        }
        if (new_state != last_outfan_state) {
            digitalWrite(RELAY_OUTFAN, (new_state == "ZAPNUTO") ? LOW : HIGH);
            last_outfan_state = new_state;
            client.publish("control/outfan", new_state.c_str(), true);
            Serial.print("mezní hodnoty pro vnější větrák: ");
            Serial.println(new_state);
        }
    }
}

// konfigurace mezních hodnot pro automatické řízení 
void configureOutfanSettings(String message) {
    message.trim();
    if (message.equals("none")) {
        threshold_temperature = NAN;
        temp_active = false;
        threshold_humidity = NAN;
        humidity_active = false;
        Serial.println("mezní hodnoty pro vnější větrák = deaktivovány");
    } else {
        // Nastavení teploty
        int temp_pos = message.indexOf("T=");
        if (temp_pos != -1) {
            int end_pos = message.indexOf("°C", temp_pos);
            if (end_pos != -1) {
                String tempVal = message.substring(temp_pos + 2, end_pos);
                threshold_temperature = tempVal.toFloat();
                temp_active = true;
                Serial.print("mez teploty: ");
                Serial.println(threshold_temperature);
            } else {
                threshold_temperature = NAN;
                temp_active = false;
            }
        } else {
            threshold_temperature = NAN;
            temp_active = false;
        }
        // Nastavení vlhkosti
        int hum_pos = message.indexOf("H=");
        if (hum_pos != -1) {
            int end_pos = message.indexOf("%", hum_pos);
            if (end_pos != -1) {
                String humVal = message.substring(hum_pos + 2, end_pos);
                threshold_humidity = humVal.toFloat();
                humidity_active = true;
                Serial.print("mez vlhkosti: ");
                Serial.println(threshold_humidity);
            } else {
                threshold_humidity = NAN;
                humidity_active = false;
            }
        } else {
            threshold_humidity = NAN;
            humidity_active = false;
        }
    }
    is_border_active = temp_active || humidity_active;
    if (is_border_active) {
        Serial.println("automatické řízení vnějšího větráku = zapnuto");
    } else {
        Serial.println("automatické řízení vnějšího větráku = vypnuto");
    }
}