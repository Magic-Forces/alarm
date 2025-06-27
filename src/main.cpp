#include <Arduino.h>
#include <RtcDS1302.h>

#define CH1 2
// #define CH2 3
#define PIR 4
#define REED 5
#define SIREN 6
// #define UPS 7
// #define PUMP 8
#define BUZZER 9
#define CLK 10
#define DAT 11
#define RST 12
#define LED 13

enum AlarmState
{
    ALARM_OFF,
    ALARM_ON,
    ALARM_TRIGGERED
};

AlarmState alarmState = ALARM_OFF;
unsigned long lastButtonPress = 0;
unsigned long alarmTriggerTime = 0;
unsigned long lastRtcCheck = 0;
unsigned long lastRtcHealthCheck = 0;
bool buttonPressed = false;
bool autoArmedToday = false;
bool rtcReliable = true;
bool rtcErrorPending = false;

const unsigned long DEBOUNCE_TIME = 100;
const unsigned long ALARM_DURATION = 180000;
const unsigned long RTC_CHECK_INTERVAL = 600000;
const unsigned long RTC_HEALTH_CHECK_INTERVAL = 1800000;

ThreeWire myWire(DAT, CLK, RST);
RtcDS1302<ThreeWire> Rtc(myWire);

void checkRemoteButton();
void checkAutoArm();
void armAlarm();
void disarmAlarm();
void triggerAlarm();
void beep(int times, int duration);
bool hasTimeElapsed(unsigned long startTime, unsigned long duration);
bool checkRtcHealth();
void tryFixRtc();

void setup()
{
    pinMode(CH1, INPUT_PULLUP);
    pinMode(PIR, INPUT_PULLUP);
    pinMode(REED, INPUT_PULLUP);
    pinMode(SIREN, OUTPUT);
    pinMode(BUZZER, OUTPUT);
    pinMode(LED, OUTPUT);

    digitalWrite(SIREN, HIGH);
    digitalWrite(BUZZER, HIGH);

    Rtc.Begin();

    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    bool rtcError = false;

    if (!Rtc.IsDateTimeValid())
    {
        rtcError = true;
        Rtc.SetDateTime(compiled);
    }
    if (Rtc.GetIsWriteProtected())
    {
        rtcError = true;
        Rtc.SetIsWriteProtected(false);
    }
    if (!Rtc.GetIsRunning())
    {
        rtcError = true;
        Rtc.SetIsRunning(true);
    }
    RtcDateTime now = Rtc.GetDateTime();
    if (now < compiled)
    {
        rtcError = true;
        Rtc.SetDateTime(compiled);
    }

    rtcReliable = !rtcError;

    if (rtcError)
    {
        rtcErrorPending = true;
    }

    triggerAlarm();
}

void loop()
{
    checkRemoteButton();

    switch (alarmState)
    {
    case ALARM_OFF:
        checkAutoArm();
        break;

    case ALARM_ON:
        if (digitalRead(PIR) == LOW || digitalRead(REED) == HIGH)
        {
            triggerAlarm();
        }
        break;

    case ALARM_TRIGGERED:
        if (hasTimeElapsed(alarmTriggerTime, ALARM_DURATION))
        {
            digitalWrite(SIREN, HIGH);
            alarmState = ALARM_ON;
        }
        break;
    }
}

bool checkRtcHealth()
{
    if (!hasTimeElapsed(lastRtcHealthCheck, RTC_HEALTH_CHECK_INTERVAL))
    {
        return rtcReliable;
    }
    lastRtcHealthCheck = millis();

    if (!Rtc.IsDateTimeValid())
    {
        return false;
    }

    if (!Rtc.GetIsRunning())
    {
        return false;
    }

    if (Rtc.GetIsWriteProtected())
    {
        return false;
    }

    return true;
}

void tryFixRtc()
{
    bool fixed = false;

    if (!Rtc.GetIsRunning())
    {
        Rtc.SetIsRunning(true);
        fixed = true;
    }

    if (Rtc.GetIsWriteProtected())
    {
        Rtc.SetIsWriteProtected(false);
        fixed = true;
    }

    if (fixed)
    {
        delay(100);
        rtcReliable = checkRtcHealth();

        if (rtcReliable)
        {
            rtcErrorPending = false;
        }
    }
}

void checkAutoArm()
{
    if (!checkRtcHealth())
    {
        if (rtcReliable)
        {
            rtcReliable = false;
            rtcErrorPending = true;
            tryFixRtc();
        }
        return;
    }

    if (!rtcReliable)
    {
        return;
    }

    if (!hasTimeElapsed(lastRtcCheck, RTC_CHECK_INTERVAL))
    {
        return;
    }
    lastRtcCheck = millis();

    RtcDateTime now = Rtc.GetDateTime();

    if (now.Hour() == 0 && autoArmedToday)
    {
        autoArmedToday = false;
    }

    if (now.Hour() >= 22 && !autoArmedToday && digitalRead(REED) == LOW)
    {
        beep(3, 150);
        delay(500);
        alarmState = ALARM_ON;
        digitalWrite(LED, LOW);
        autoArmedToday = true;
    }
}

void checkRemoteButton()
{
    if (digitalRead(CH1) == LOW && !buttonPressed && hasTimeElapsed(lastButtonPress, DEBOUNCE_TIME))
    {
        buttonPressed = true;
        lastButtonPress = millis();

        if (alarmState == ALARM_OFF)
        {
            armAlarm();
        }
        else
        {
            disarmAlarm();
        }
    }
    if (digitalRead(CH1) == HIGH)
    {
        buttonPressed = false;
    }
}

bool hasTimeElapsed(unsigned long startTime, unsigned long duration)
{
    return (millis() - startTime) >= duration;
}

void armAlarm()
{
    if (digitalRead(REED) == HIGH)
    {
        beep(2, 1000);
        return;
    }
    alarmState = ALARM_ON;
    digitalWrite(LED, LOW);
    beep(2, 200);
}

void triggerAlarm()
{
    alarmState = ALARM_TRIGGERED;
    alarmTriggerTime = millis();
    digitalWrite(SIREN, LOW);
}

void disarmAlarm()
{
    alarmState = ALARM_OFF;
    digitalWrite(SIREN, HIGH);
    digitalWrite(LED, HIGH);

    beep(1, 200);

    if (rtcErrorPending)
    {
        delay(300);
        beep(1, 1000);
        rtcErrorPending = false;
    }

    if (rtcReliable && checkRtcHealth())
    {
        RtcDateTime now = Rtc.GetDateTime();
        if (now.Hour() >= 22)
        {
            autoArmedToday = false;
        }
    }
}

void beep(int times, int duration)
{
    if (duration > 5000)
        duration = 5000;
    for (int i = 0; i < times; i++)
    {
        digitalWrite(BUZZER, LOW);
        delay(duration);
        digitalWrite(BUZZER, HIGH);
        if (i < times - 1)
        {
            delay(100);
        }
    }
}