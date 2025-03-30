#include "TimeLib.h"
#include "infan_control.h"
#include "pinout.h"
#include "wifi_mqtt_config.h"

bool interval_active = false;
int infan_interval_minutes = 0;
unsigned long last_infan_check_millis = 0;
bool infan_relay_state = false;

// manuální řížení pomocí ON/OFF checkoboxu
void controlInfanManually(String message) {
  if (!interval_active) {
    bool relay_on = (message == "ZAPNUTO");
    digitalWrite(RELAY_INFAN, relay_on ? LOW : HIGH);
    infan_relay_state = relay_on;
    Serial.print("vnitřní větrák manuálně: ");
    Serial.println(relay_on ? "ZAPNUTO" : "VYPNUTO");
  }
}

// automatické řízení vnitřního větráku dle nastaveného intervalu
void controlInfanBasedOnSettings() {
  unsigned long current_millis = millis();
  if (current_millis - last_infan_check_millis >= 60000) {
      last_infan_check_millis = current_millis;
    if (interval_active) {
      int current_minute_in_hour = minute();
      if (current_minute_in_hour < infan_interval_minutes) {
        if (!infan_relay_state) {
          digitalWrite(RELAY_INFAN, LOW);
          client.publish("control/infan", "ZAPNUTO", true);
          infan_relay_state = true;
          Serial.println("vnitřní větrák = zapnutý (dle intervalu)");
        }
      } else {
        if (infan_relay_state) {
          digitalWrite(RELAY_INFAN, HIGH);
          client.publish("control/infan", "VYPNUTO", true);
          infan_relay_state = false;
          Serial.println("vnitřní větrák = vypnutý (dle intrervalu)");
        }
      }
    }
  }
}

// zpracování zprávy o nastavení automatického řízení dle počtu aktivních minut v hodině
void processInfanInterval(String message) {
  message.trim();
  if (message.equals("none")) {
      interval_active = false;
      infan_interval_minutes = 0;
    Serial.println("interval pro vnitřní větrák = vypnut --> přepínám na manuální ovládání");
  } else {
      infan_interval_minutes = message.toInt();
    if (infan_interval_minutes > 0 && infan_interval_minutes <= 60) {
        interval_active = true;
      Serial.print("interval pro vnitřní větrák: ");
      Serial.print(infan_interval_minutes);
      Serial.println(" minut každou hodinu.");
    } else {
        interval_active = false;
      Serial.println("neplatná hodnota intervalu (více než 60 / záporné)--> ignoruji");
    }
  }
}