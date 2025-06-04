#ifndef AUDIOFEEDBACK_H
#define AUDIOFEEDBACK_H

#include <Arduino.h>

#define ALARM_TONE 1500
#define PUMP_TONE 1000
#define BEEP_DURATION 200
#define BEEP_PAUSE 100

void playBeep(String device, bool state, int signalPin);

#endif