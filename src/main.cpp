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
bool ch1_last_state = HIGH;
unsigned long ch1_debounce_time = 0;

bool pump_state = false;
bool ch2_current_state;
bool ch2_last_state = HIGH;
unsigned long ch2_debounce_time = 0;

bool last_ups_state = true;

unsigned long valve_timer = 0;
bool valve_opening = false;
bool valve_closing = false;

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

void handleButtons()
{
  bool ch1_reading = digitalRead(ch1);

  if (ch1_reading != ch1_last_state)
  {
    ch1_debounce_time = millis();
  }

  if ((millis() - ch1_debounce_time) > DEBOUNCE)
  {
    if (ch1_reading != ch1_current_state)
    {
      ch1_current_state = ch1_reading;

      if (ch1_current_state == LOW)
      {
        if (isContinuousAlarmActive())
        {
          stopContinuousAlarm(signal);

          if (alarm_state == true)
          {
            alarm_state = false;
            playBeep(DEVICE_ALARM, alarm_state, signal);

            if (pump_state == true)
            {
              pump_state = false;
            }
          }
        }
        else
        {
          alarm_state = !alarm_state;
          playBeep(DEVICE_ALARM, alarm_state, signal);

          if (alarm_state == true && pump_state == true)
          {
            pump_state = false;
          }
        }
      }
    }
  }
  ch1_last_state = ch1_reading;

  if (!isContinuousAlarmActive())
  {
    bool ch2_reading = digitalRead(ch2);

    if (ch2_reading != ch2_last_state)
    {
      ch2_debounce_time = millis();
    }

    if ((millis() - ch2_debounce_time) > DEBOUNCE)
    {
      if (ch2_reading != ch2_current_state)
      {
        ch2_current_state = ch2_reading;

        if (ch2_current_state == LOW && alarm_state == false)
        {
          pump_state = !pump_state;
          playBeep(DEVICE_PUMP, pump_state, signal);
        }
      }
    }
    ch2_last_state = ch2_reading;
  }
}

void handleAlarmTriggers()
{
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
}

void handleOutputs()
{
  digitalWrite(led, alarm_state ? HIGH : LOW);

  static bool pump_running = false;

  if (pump_state == true && alarm_state == false)
  {
    if (!pump_running && !valve_opening)
    {
      digitalWrite(valve, HIGH);
      valve_timer = millis();
      valve_opening = true;
    }
    else if (valve_opening && (millis() - valve_timer >= VALVE_DELAY))
    {
      digitalWrite(pump, HIGH);
      pump_running = true;
      valve_opening = false;
    }
  }
  else
  {
    if (valve_opening)
    {
      valve_opening = false;
      digitalWrite(valve, LOW);
      valve_timer = millis();
      valve_closing = true;
    }
    else if (pump_running)
    {
      digitalWrite(pump, LOW);
      pump_running = false;
      valve_timer = millis();
      valve_closing = true;
    }
    else if (valve_closing && (millis() - valve_timer >= VALVE_DELAY))
    {
      digitalWrite(valve, LOW);
      valve_closing = false;
    }
  }
}

void loop()
{
  handleContinuousAlarm(signal);
  handleButtons();
  handleAlarmTriggers();
  handleOutputs();
}