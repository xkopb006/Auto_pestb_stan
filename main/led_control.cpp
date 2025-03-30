#include <TimeLib.h>
#include "led_control.h"
#include "pinout.h"
#include "wifi_mqtt_config.h"

bool schedule_is_active = false;
int schedule_start_minutes = 0;
int schedule_end_minutes = 0;
String last_led_state = "VYPNUTO";

// manuální řížení pomocí ON/OFF checkoboxu
void controlLEDManually(String message) {
    if (!schedule_is_active) {
        String new_state = message;
        if (new_state != last_led_state) {
            digitalWrite(RELAY_LED, (new_state == "ZAPNUTO") ? LOW : HIGH);
            last_led_state = new_state;
        } else {
            digitalWrite(RELAY_LED, (new_state == "ZAPNUTO") ? LOW : HIGH);
        }
    }
}

// parsování zprávy o nastavení automatického řízení dle časového intervalu 
void processScheduleMessage(String message) {
    if (message == "none" || message == "delete") {
        schedule_is_active = false;
        Serial.println("plán pro LED = smazáno --> přepínám na manuální ovládání");
    } else {
        int dashIndex = message.indexOf("-");
        if (dashIndex > 0) {
            String start_str = message.substring(0, dashIndex);
            String end_str = message.substring(dashIndex + 1);
            int start_hour = start_str.substring(0, 2).toInt();
            int start_min = start_str.substring(3, 5).toInt();
            int end_hour = end_str.substring(0, 2).toInt();
            int end_min = end_str.substring(3, 5).toInt();
            schedule_start_minutes = start_hour * 60 + start_min;
            schedule_end_minutes = end_hour * 60 + end_min;
            schedule_is_active = true;
            Serial.print("nový plán pro LED: ");
            Serial.print(schedule_start_minutes);
            Serial.print(" - ");
            Serial.println(schedule_end_minutes);
        }
    }
}

// automatické řízení LED pomocí zadaného nastavení "rozvrhu"
void controlLEDBySchedule() {
  if (schedule_is_active) {
        int current_minutes = hour() * 60 + minute();
        Serial.print("Aktuální čas (min): ");
        Serial.println(current_minutes);
        
        bool led_on = false;
        // interval nepřesahuje půlnoc
        if (schedule_start_minutes < schedule_end_minutes) {
            led_on = (current_minutes >= schedule_start_minutes && current_minutes < schedule_end_minutes);
        }
        // interval překračuje půlnoc
        else {
            led_on = (current_minutes >= schedule_start_minutes || current_minutes < schedule_end_minutes);
        }
        
        if (led_on) {
            digitalWrite(RELAY_LED, LOW);
            client.publish("control/led", "ZAPNUTO", true);
            last_led_state = "ZAPNUTO";
        } else {
            digitalWrite(RELAY_LED, HIGH);
            client.publish("control/led", "VYPNUTO", true);
            last_led_state = "VYPNUTO";
        }
    }
}