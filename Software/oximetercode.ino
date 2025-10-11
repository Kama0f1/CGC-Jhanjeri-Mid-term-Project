#include <Wire.h>
#include "MAX30105.h"
#include <LiquidCrystal_I2C.h>

MAX30105 sensor;
LiquidCrystal_I2C lcd(0x27,16,2);

float bpm = 0;
float spo2 = 0;

void setup() {
  lcd.init();
  lcd.backlight();
  sensor.begin(Wire, I2C_SPEED_STANDARD);
  sensor.setup(0x1F,4,2,100,411,4096);
}

void loop() {
  bpm = sensor.getHeartRate();
  spo2 = sensor.getSpO2();

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("BPM:");
  lcd.print((int)bpm);
  lcd.print("%");
  lcd.setCursor(0,1);
  lcd.print("SpO2:");
  lcd.print((int)spo2);
  lcd.print("%");
  delay(1000);
}
