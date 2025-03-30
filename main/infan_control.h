// SOUBOR PRO VNITŘNÍ VĚTRÁK
#ifndef INFAN_CONTROL_H
#define INFAN_CONTROL_H

#include <Arduino.h>

extern bool interval_active;
extern int infan_interval_minutes;
extern unsigned long last_infan_check_millis;
extern bool infan_relay_state;

void controlInfanManually(String message);
void processInfanInterval(String message);
void controlInfanBasedOnSettings();

#endif
