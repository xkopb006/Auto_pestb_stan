// SOUBOR PRO REGULACI pH
#ifndef PH_CONTROL_H
#define PH_CONTROL_H

#include <Arduino.h>

extern float ph_min_threshold;
extern float ph_max_threshold;
extern bool ph_control_active;
extern unsigned long ph_plus_activated_time;
extern bool ph_plus_action_in_progress;
extern unsigned long ph_minus_activated_time;
extern bool ph_minus_action_in_progress;
unsigned long last_PH_trigger_time;

int getStableAnalogValue(uint8_t num_samples, uint8_t skip);
float convertToPH(int analog_value);
float getPHValue();
void controlPHByThresholds(String message);
void processPHThresholds();

#endif