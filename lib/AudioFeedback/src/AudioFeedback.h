#ifndef AUDIOFEEDBACK_H
#define AUDIOFEEDBACK_H

#include <Arduino.h>

#define ALARM_TONE 1500
#define PUMP_TONE 1000
#define BEEP_DURATION 200
#define BEEP_PAUSE 100

#define CONTINUOUS_ALARM_TONE 800
#define CONTINUOUS_ALARM_DURATION 180000

void playBeep(String device, bool state, int signalPin);
void handleContinuousAlarm(int signalPin);
void startContinuousAlarm();
void stopContinuousAlarm(int signalPin);
bool isContinuousAlarmActive();

#endif