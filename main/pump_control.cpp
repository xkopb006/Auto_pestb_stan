#include "pump_control.h"
#include "pinout.h"
#include "wifi_mqtt_config.h"

// manuální řížení pomocí ON/OFF checkoboxu
void controlPumpManually(String message) {
  bool relay_on = (message == "ZAPNUTO");
  digitalWrite(RELAY_PUMP, relay_on ? LOW : HIGH);
}
