#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

#define ch1 2
#define ch2 3
#define ups 4
#define led 5
#define pump 6
#define valve 7
#define signal 8

#define pir0 A0
#define pir1 A1

#define VALVE_DELAY 2000
#define DEBOUNCE 50

LiquidCrystal_I2C lcd(0x27, 16, 2);

byte bell[] = {
    B00100, // ..#..
    B01110, // .###.
    B01110, // .###.
    B01110, // .###.
    B11111, // #####
    B11111, // #####
    B00000, // .....
    B00100  // ..#..
};

bool alarm_state = true;
bool ch1_current_state;
bool ch1_last_state = !ch1_current_state;

bool pump_state = false;
bool ch2_current_state;
bool ch2_last_state = !ch2_current_state;

void setup()
{
  pinMode(ch1, INPUT_PULLUP);
  pinMode(ch2, INPUT_PULLUP);
  pinMode(ups, INPUT);
  pinMode(pir0, INPUT);
  pinMode(pir1, INPUT);

  pinMode(led, OUTPUT);
  pinMode(pump, OUTPUT);
  pinMode(valve, OUTPUT);
  pinMode(signal, OUTPUT);

  lcd.init();
  lcd.createChar(0, bell);

  lcd.setCursor(0, 0);
  lcd.print("ALARM:");
  lcd.setCursor(0, 1);
  lcd.print("POMPA:");
}

void loop()
{
  ch1_current_state = digitalRead(ch1);
  if (ch1_last_state == HIGH && ch1_current_state == LOW)
  {
    alarm_state = !alarm_state;
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
    }
    delay(DEBOUNCE);
  }
  ch2_last_state = ch2_current_state;

  if (alarm_state == true)
  {
    lcd.setCursor(7, 0);
    lcd.noBacklight();
    digitalWrite(led, HIGH);
    lcd.print("ON ");
  }
  else
  {
    lcd.setCursor(7, 0);
    lcd.backlight();
    digitalWrite(led, LOW);
    lcd.print("OFF");
  }

  if (pump_state == true && alarm_state == false)
  {
    lcd.setCursor(7, 1);
    digitalWrite(valve, HIGH);
    // delay(VALVE_DELAY);
    digitalWrite(pump, HIGH);
    lcd.print("ON ");
  }
  else
  {
    lcd.setCursor(7, 1);
    digitalWrite(pump, LOW);
    // delay(VALVE_DELAY);
    digitalWrite(valve, LOW);
    lcd.print("OFF");
  }
}