#include "AudioFeedback.h"

void playBeep(String device, bool state, int signalPin)
{
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