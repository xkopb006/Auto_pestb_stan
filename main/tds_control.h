// SOUBOR PRO REGULACI TDS
#ifndef TDS_CONTROL_H
#define TDS_CONTROL_H

#include <Arduino.h>

extern float tds_min_threshold;
unsigned long last_TDS_regulation;
const unsigned long TDS_interval;

bool tds_pump_active;
unsigned long tds_pump_start_time;

void setTdsMinThreshold(String message);

void processTDSThreshold();

#endif
