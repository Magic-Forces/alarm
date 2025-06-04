#include "AudioFeedback.h"

static bool continuousAlarmActive = false;
static unsigned long alarmStartTime = 0;
static unsigned long lastToneTime = 0;
static bool toneState = false;

void playBeep(String device, bool state, int signalPin)
{
    if (continuousAlarmActive)
        return;

    int tone_freq;
    int beep_count;

    if (device == "alarm")
    {
        tone_freq = ALARM_TONE;
        beep_count = state ? 2 : 1;
    }
    else if (device == "pump")
    {
        tone_freq = PUMP_TONE;
        beep_count = state ? 1 : 2;
    }
    else
    {
        return;
    }

    for (int i = 0; i < beep_count; i++)
    {
        tone(signalPin, tone_freq, BEEP_DURATION);
        delay(BEEP_DURATION);
        if (i < beep_count - 1)
        {
            delay(BEEP_PAUSE);
        }
    }
}

void startContinuousAlarm()
{
    if (!continuousAlarmActive)
    {
        continuousAlarmActive = true;
        alarmStartTime = millis();
        lastToneTime = 0;
        toneState = false;
    }
}

void stopContinuousAlarm(int signalPin)
{
    continuousAlarmActive = false;
    noTone(signalPin);
}

bool isContinuousAlarmActive()
{
    return continuousAlarmActive;
}

void handleContinuousAlarm(int signalPin)
{
    if (!continuousAlarmActive)
        return;

    unsigned long currentTime = millis();

    if (currentTime - alarmStartTime >= CONTINUOUS_ALARM_DURATION)
    {
        stopContinuousAlarm(signalPin);
        return;
    }

    if (currentTime - lastToneTime >= 500)
    {
        if (toneState)
        {
            noTone(signalPin);
            toneState = false;
        }
        else
        {
            tone(signalPin, CONTINUOUS_ALARM_TONE);
            toneState = true;
        }
        lastToneTime = currentTime;
    }
}