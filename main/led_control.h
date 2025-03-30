// SOUBOR PRO LED OSVĚTLENÍ
#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <Arduino.h>

extern bool is_schedule_active;
extern int schedule_start_in_minutes;
extern int schedule_end_in_minutes;
extern String last_led_state;

void controlLEDManually(String message);
void processScheduleMessage(String message);
void controlLEDBySchedule();

#endif