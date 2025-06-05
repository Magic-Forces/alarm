#include "AudioFeedback.h"

static bool continuousAlarmActive = false;
static unsigned long alarmStartTime = 0;
static unsigned long lastToneTime = 0;
static bool toneState = false;

static bool beepActive = false;
static unsigned long beepStartTime = 0;
static int beepCount = 0;
static int targetBeeps = 0;
static int beepTone = 0;
static int beepPin = 0;
static bool beepOn = false;
static unsigned long beepStateTime = 0;

void playBeep(DeviceType device, bool state, int signalPin)
{
    if (continuousAlarmActive || beepActive)
        return;

    if (device == DEVICE_ALARM)
    {
        beepTone = ALARM_TONE;
        targetBeeps = state ? 2 : 1;
    }
    else if (device == DEVICE_PUMP)
    {
        beepTone = PUMP_TONE;
        targetBeeps = state ? 1 : 2;
    }
    else
    {
        return;
    }

    beepActive = true;
    beepPin = signalPin;
    beepCount = 0;
    beepOn = false;
    beepStartTime = millis();
    beepStateTime = millis();
}

void handleBeep()
{
    if (!beepActive)
        return;

    unsigned long currentTime = millis();

    if (!beepOn && beepCount < targetBeeps)
    {
        tone(beepPin, beepTone);
        beepOn = true;
        beepStateTime = currentTime;
    }
    else if (beepOn && (currentTime - beepStateTime >= BEEP_DURATION))
    {
        noTone(beepPin);
        beepOn = false;
        beepCount++;
        beepStateTime = currentTime;

        if (beepCount >= targetBeeps)
        {
            beepActive = false;
        }
    }
    else if (!beepOn && beepCount < targetBeeps && (currentTime - beepStateTime >= BEEP_PAUSE))
    {
        beepStateTime = currentTime;
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
    handleBeep();

    if (!continuousAlarmActive)
        return;

    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - alarmStartTime;

    if (elapsedTime >= CONTINUOUS_ALARM_DURATION)
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