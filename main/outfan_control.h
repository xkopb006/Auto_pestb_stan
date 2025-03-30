// SOUBOR PRO VNĚJŠÍ VĚTRÁK
#ifndef OUTFAN_CONTROL_H
#define OUTFAN_CONTROL_H

#include <Arduino.h>

extern bool is_border_active;
extern bool temp_active;
extern bool humidity_active;
extern String last_outfan_state;
extern float threshold_temperature;
extern float threshold_humidity;

void controlOutfanBasedOnConditions(float temperature, float humidity);
void controlOutfanManually(String message);
void configureOutfanSettings(String message);

void controlOutfanBasedOnConditions(float temperature, float humidity);
void controlOutfanManually(String message);
void configureOutfanSettings(String message);

#endif 