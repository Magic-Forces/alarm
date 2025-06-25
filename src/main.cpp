#include <Arduino.h>
#include <RtcDS1302.h>

#define CH1 2 // RF receiver output (push button from remote)
// #define CH2 3
#define PIR 4       // PIR sensor input (LOW when motion detected)
#define REED 5      // Reed sensor input (LOW when door closed, HIGH when door opened)
#define SIREN_PIN 6 // Alarm siren output pin (LOW = relay ON, HIGH = relay OFF)
// #define UPS 7
// #define PUMP 8
#define BUZZER 9 // Buzzer pin for state change notifications (LOW = relay ON, HIGH = relay OFF)
#define CLK 10   // DS1302 Clock pin
#define DAT 11   // DS1302 Data pin
#define RST 12   // DS1302 Reset pin
#define LED 13   // LED indicator (LOW = alarm on, HIGH = alarm off)

// System states
enum AlarmState
{
    ALARM_OFF,      // System disarmed
    ALARM_ARMED,    // System armed, waiting for trigger
    ALARM_TRIGGERED // Alarm triggered, siren active
};

// Global variables
AlarmState alarmState = ALARM_OFF;  // Current alarm state
unsigned long lastButtonPress = 0;  // Timestamp of last remote button press
unsigned long alarmTriggerTime = 0; // Timestamp when alarm was triggered
unsigned long lastRtcCheck = 0;     // Timestamp of last auto-arm time check
bool buttonPressed = false;         // Debounce flag for remote button
bool autoArmedToday = false;        // Flag to ensure auto-arm happens only once per day

// Timing constants (milliseconds)
const unsigned long DEBOUNCE_TIME = 100;         // Debounce time for RF receiver
const unsigned long ALARM_DURATION = 180000;     // Siren duration:  3 minutes
const unsigned long RTC_CHECK_INTERVAL = 600000; // Auto-arm check interval: 10 minutes

// RTC setup
ThreeWire myWire(DAT, CLK, RST);  // ThreeWire interface for DS1302
RtcDS1302<ThreeWire> Rtc(myWire); // RTC instance

// Function declarations
void checkRemoteButton();                                             // Check if remote button was pressed
void checkAutoArm();                                                  // Auto-arm logic after 22:00
void armAlarm();                                                      // Manually arm the system
void disarmAlarm();                                                   // Disarm the system
void triggerAlarm();                                                  // Trigger the siren
void beep(int times, int duration);                                   // Sound buzzer a number of times
bool hasTimeElapsed(unsigned long startTime, unsigned long duration); // Timer helper

void setup()
{
    // Configure input and output pins
    pinMode(CH1, INPUT_PULLUP);
    pinMode(PIR, INPUT_PULLUP);
    pinMode(REED, INPUT_PULLUP);
    pinMode(SIREN_PIN, OUTPUT);
    pinMode(BUZZER, OUTPUT);
    pinMode(LED, OUTPUT);

    // Initialize outputs to default OFF state (HIGH = relay OFF for LOW-active relays)
    digitalWrite(SIREN_PIN, HIGH); // Relay OFF - siren disabled
    digitalWrite(BUZZER, HIGH);    // Relay OFF - buzzer disabled

    // Initialize RTC module
    Rtc.Begin();

    // --- RTC synchronization with compile time ---
    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    if (!Rtc.IsDateTimeValid())
    {
        Rtc.SetDateTime(compiled); // Set to compile time if RTC invalid
    }
    if (Rtc.GetIsWriteProtected())
    {
        Rtc.SetIsWriteProtected(false); // Allow writing time
    }
    if (!Rtc.GetIsRunning())
    {
        Rtc.SetIsRunning(true); // Start RTC oscillator if stopped
    }
    RtcDateTime now = Rtc.GetDateTime();
    if (now < compiled)
    {
        Rtc.SetDateTime(compiled); // Correct RTC if it's behind compile time
    }
    // --- End RTC synchronization ---

    // Immediately trigger alarm on startup
    triggerAlarm();
}

void loop()
{
    checkRemoteButton(); // Handle remote arming/disarming
    checkAutoArm();      // Handle automatic arming at night

    switch (alarmState)
    {
    case ALARM_OFF:
        // Do nothing when system is disarmed
        break;

    case ALARM_ARMED:
        // If motion detected or door opened, trigger alarm
        if (digitalRead(PIR) == LOW || digitalRead(REED) == HIGH)
        {
            triggerAlarm();
        }
        break;

    case ALARM_TRIGGERED:
        // After siren duration, stop siren and revert to ARMED
        if (hasTimeElapsed(alarmTriggerTime, ALARM_DURATION))
        {
            digitalWrite(SIREN_PIN, HIGH); // Turn OFF siren relay
            alarmState = ALARM_ARMED;
        }
        break;
    }
}

void checkAutoArm()
{
    // Only check RTC periodically
    if (!hasTimeElapsed(lastRtcCheck, RTC_CHECK_INTERVAL))
    {
        return;
    }
    lastRtcCheck = millis();

    RtcDateTime now = Rtc.GetDateTime();

    // Reset daily flag at midnight
    if (now.Hour() == 0 && autoArmedToday)
    {
        autoArmedToday = false;
    }

    // Conditions: after 22:00, not yet auto-armed today, system OFF, door closed
    if (now.Hour() >= 22 && !autoArmedToday && alarmState == ALARM_OFF && digitalRead(REED) == LOW)
    {
        beep(3, 150);
        delay(500);
        alarmState = ALARM_ARMED;
        digitalWrite(LED, LOW);
        autoArmedToday = true;
    }
}

void checkRemoteButton()
{
    // Debounce and detect falling edge on remote button
    if (digitalRead(CH1) == LOW && !buttonPressed && hasTimeElapsed(lastButtonPress, DEBOUNCE_TIME))
    {
        buttonPressed = true;
        lastButtonPress = millis();

        // Toggle alarm state on button press
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
        buttonPressed = false; // Reset debounce flag
    }
}

bool hasTimeElapsed(unsigned long startTime, unsigned long duration)
{
    return (millis() - startTime) >= duration; // Return true if specified duration passed
}

void armAlarm()
{
    // Prevent arming if door is open
    if (digitalRead(REED) == HIGH)
    {
        beep(1, 1000);
        return;
    }
    alarmState = ALARM_ARMED;
    digitalWrite(LED, LOW);
    beep(1, 200);
}

void triggerAlarm()
{
    alarmState = ALARM_TRIGGERED;
    alarmTriggerTime = millis();
    digitalWrite(SIREN_PIN, LOW); // Turn ON siren relay (LOW = active)
}

void disarmAlarm()
{
    alarmState = ALARM_OFF;
    digitalWrite(SIREN_PIN, HIGH); // Turn OFF siren relay (HIGH = inactive)
    digitalWrite(LED, HIGH);
    beep(2, 200);
    RtcDateTime now = Rtc.GetDateTime();
    // Reset daily auto-arm flag if after 22:00
    if (now.Hour() >= 22)
    {
        autoArmedToday = false;
    }
}

void beep(int times, int duration)
{
    // Limit beep duration to maximum
    if (duration > 5000)
        duration = 5000;
    for (int i = 0; i < times; i++)
    {
        digitalWrite(BUZZER, LOW); // Turn ON buzzer relay
        delay(duration);
        digitalWrite(BUZZER, HIGH); // Turn OFF buzzer relay
        if (i < times - 1)          // Don't delay after last beep
        {
            delay(100);
        }
    }
}
