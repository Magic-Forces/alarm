#include <Arduino.h>
#include <AudioFeedback.h>

#define ch1 2
#define ch2 3
#define ups 4
#define led 5
#define valve 6
#define pump 7
#define signal 8

#define pir0 A0
#define pir1 A1

#define VALVE_DELAY 2000
#define DEBOUNCE 50
#define PIR_THRESHOLD 500

bool alarm_state = true;
bool ch1_current_state;
bool ch1_last_state;

bool pump_state = false;
bool ch2_current_state;
bool ch2_last_state;

bool last_ups_state = true;

void setup()
{
  pinMode(ch1, INPUT_PULLUP);
  pinMode(ch2, INPUT_PULLUP);
  pinMode(ups, INPUT_PULLUP);
  pinMode(pir0, INPUT);
  pinMode(pir1, INPUT);

  pinMode(led, OUTPUT);
  pinMode(pump, OUTPUT);
  pinMode(valve, OUTPUT);
  pinMode(signal, OUTPUT);
}

void loop()
{
  handleContinuousAlarm(signal);

  if (isContinuousAlarmActive())
  {
    ch1_current_state = digitalRead(ch1);
    if (ch1_last_state == HIGH && ch1_current_state == LOW)
    {
      stopContinuousAlarm(signal);

      if (alarm_state == true)
      {
        alarm_state = false;

        delay(100);
        playBeep("alarm", alarm_state, signal);

        if (pump_state == true)
        {
          pump_state = false;
        }
      }

      delay(DEBOUNCE);
    }
    ch1_last_state = ch1_current_state;
  }

  if (!isContinuousAlarmActive())
  {
    ch1_current_state = digitalRead(ch1);
    if (ch1_last_state == HIGH && ch1_current_state == LOW)
    {
      alarm_state = !alarm_state;
      playBeep("alarm", alarm_state, signal);

      if (alarm_state == true && pump_state == true)
      {
        pump_state = false;
      }
      delay(DEBOUNCE);
    }
    ch1_last_state = ch1_current_state;

    ch2_current_state = digitalRead(ch2);
    if (ch2_last_state == HIGH && ch2_current_state == LOW)
    {
      if (alarm_state == false)
      {
        pump_state = !pump_state;
        playBeep("pump", pump_state, signal);
      }
      delay(DEBOUNCE);
    }
    ch2_last_state = ch2_current_state;
  }

  if (alarm_state == true && !isContinuousAlarmActive())
  {
    int pir0_value = analogRead(pir0);
    int pir1_value = analogRead(pir1);
    bool ups_state = digitalRead(ups);

    if (pir0_value > PIR_THRESHOLD || pir1_value > PIR_THRESHOLD || ups_state == LOW)
    {
      startContinuousAlarm();
    }
  }

  bool current_ups_state = digitalRead(ups);

  if (alarm_state == false)
  {
    if (current_ups_state == LOW && last_ups_state == true)
    {
      if (pump_state == true)
      {
        pump_state = false;
      }
      startContinuousAlarm();
    }

    if (current_ups_state == HIGH && last_ups_state == false && isContinuousAlarmActive())
    {
      stopContinuousAlarm(signal);
    }
  }

  last_ups_state = current_ups_state;

  if (alarm_state == true)
  {
    digitalWrite(led, HIGH);
  }
  else
  {
    digitalWrite(led, LOW);
  }

  if (pump_state == true && alarm_state == false)
  {
    digitalWrite(valve, HIGH);
    delay(VALVE_DELAY);
    digitalWrite(pump, HIGH);
  }
  else
  {
    digitalWrite(pump, LOW);
    delay(VALVE_DELAY);
    digitalWrite(valve, LOW);
  }
}