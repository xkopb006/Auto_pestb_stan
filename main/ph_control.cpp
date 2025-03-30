#include "ph_control.h"
#include "pinout.h"
#include "wifi_mqtt_config.h"

float ph_min_threshold = NAN;
float ph_max_threshold = NAN;
bool ph_control_active = false;

unsigned long ph_plus_activated_time = 0;
bool ph_plus_action_in_progress = false;

unsigned long ph_minus_activated_time = 0;
bool ph_minus_action_in_progress = false;

unsigned long last_PH_trigger_time = 0;

#define VOLTAGE 5.0
#define ADC_RES 1023.0
#define PH_REF -5.7
float calib_val = 24.7;        // 33 zjistit z kalibrace standardizovanými roztoky

int getStableAnalogValue(uint8_t num_samples = 10, uint8_t skip = 2){
    int samples[10];

    for(int i = 0; i < num_samples; i++){
        samples[i] = analogRead(PH_SENSOR);
        delay(10);
    }

    for(int i = 0; i < num_samples - 1; i++){
        for(int j = 0; j < num_samples; j++){
            if (samples[i] > samples[j]){
                int tmp  = samples[i];
                samples[i] = samples[j];
                samples[j] = tmp;
            }
        }
    }
    long sum = 0;
    for(int i = skip-1; i < num_samples - skip; i++){
        sum += samples[i];
    }
    int avg_analog = sum / (num_samples - 2 * skip);
    return avg_analog;
}

float convertToPH(int analog_value){
    float sensor_voltage = analog_value * (VOLTAGE / ADC_RES);
    return (PH_REF * sensor_voltage + calib_val);
}

float getPHValue(){
    int stable_analog = getStableAnalogValue();
    return convertToPH(stable_analog);
}


// zpracování zprávy o nastavení mezních hodnot pro automatické řízení pH
void controlPHByThresholds(String message) {
    message.trim();
    if (message.equals("none")) {
        ph_control_active = false;
        digitalWrite(H_BRIDGE_PH_PLUS, LOW);
        digitalWrite(H_BRIDGE_PH_MINUS, LOW);
        Serial.println("pH ovládání = vypnuto");
    } else {
        int separator_pos = message.indexOf('&');
        if (separator_pos != -1) {
            String ph_min_str = message.substring(0, separator_pos);
            String ph_max_str = message.substring(separator_pos + 1);
            ph_min_threshold = ph_min_str.toFloat();
            ph_max_threshold = ph_max_str.toFloat();
            ph_control_active = true;
            Serial.print("pH ovládání = zapnuto, ph_min: ");
            Serial.print(ph_min_threshold);
            Serial.print(", ph_max: ");
            Serial.println(ph_max_threshold);
        }
    }
}

// automatická regulace pH podle toho jestli je hodnota v zadaném rozmezí
void processPHThresholds() {
    if (ph_plus_action_in_progress && (millis() - ph_plus_activated_time >= 5000)) {
        digitalWrite(H_BRIDGE_PH_PLUS, LOW);
        ph_plus_action_in_progress = false;
        Serial.println("pH+ = vypínání");
    }
    if (ph_minus_action_in_progress && (millis() - ph_minus_activated_time >= 5000)) {
        digitalWrite(H_BRIDGE_PH_MINUS, LOW);
        ph_minus_action_in_progress = false;
        Serial.println("pH- = vypínání");
    }

    if (millis() - last_PH_trigger_time < 125000) {
        return;
    }
    last_PH_trigger_time = millis();

    if (ph_control_active) {
        float ph_value = getPHValue();
        Serial.print("aktuální pH: ");
        Serial.println(ph_value);

        // pH nízké – aktivuj pH+ na 5 sekund
        if (ph_value < ph_min_threshold && !ph_plus_action_in_progress) {
            digitalWrite(H_BRIDGE_PH_PLUS, HIGH);
            digitalWrite(H_BRIDGE_PH_MINUS, LOW);
            ph_plus_action_in_progress = true;
            ph_plus_activated_time = millis();
            Serial.println("pH+ na 5 sekund = zapnuto");
        }
            // pH vysoké – aktivuj pH- na 5 sekund
        else if (ph_value > ph_max_threshold && !ph_minus_action_in_progress) {
            digitalWrite(H_BRIDGE_PH_MINUS, HIGH);
            digitalWrite(H_BRIDGE_PH_PLUS, LOW);
            ph_minus_action_in_progress = true;
            ph_minus_activated_time = millis();
            Serial.println("pH- na 5 sekund = zapnuto");
        }
    }
}