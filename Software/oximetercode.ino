#include <Wire.h>
#include "MAX30105.h" 
#include <LiquidCrystal_I2C.h> 

#include "heartRate.h"    
#include "spo2_algorithm.h" 

MAX30105 particleSensor;

LiquidCrystal_I2C lcd(0x27, 16, 2); 

#define BUFFER_SIZE 100 
uint32_t irBuffer[BUFFER_SIZE]; 
uint32_t redBuffer[BUFFER_SIZE]; 

int32_t spo2 = 0; 
int8_t validSPO2 = 0; 
int32_t maximHeartRate = 0; 
int8_t validHeartRate = 0; 

const uint32_t FINGER_ON_THRESHOLD = 50000; 
bool fingerWasPresent = false; 

unsigned long lastDisplayTime = 0;
const long displayInterval = 500; 

void initializeLCD();
void initializeSensor();
void displayWaitingScreen();
void displayReadingScreen();

void setup() {
  Serial.begin(115200);
  Serial.println("Patient Health Monitor Starting...");

  Wire.begin(D2, D1); 

  initializeLCD();

  initializeSensor();

  lcd.setCursor(0, 1);
  lcd.print("Initializing.");
  
  for (int i = 0; i < BUFFER_SIZE; i++) {
    while (particleSensor.available() == false) { particleSensor.check(); }
    
    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();

    particleSensor.nextSample();
    
    if (i % 25 == 0) { 
        lcd.print("."); 
    }
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place Finger");
  lcd.setCursor(0, 1);
  lcd.print("Ready.");
}

void loop() {
  particleSensor.check();

  if (particleSensor.available() == false) {
    yield();
    return;
  }
  
  uint32_t irValue = particleSensor.getIR(); 
  uint32_t redValue = particleSensor.getRed();
  particleSensor.nextSample();

  Serial.print("IR Value: "); 
  Serial.println(irValue);


  if (irValue > FINGER_ON_THRESHOLD) {
    
    fingerWasPresent = true;

    for (int i = 0; i < BUFFER_SIZE - 1; i++) {
      redBuffer[i] = redBuffer[i+1];
      irBuffer[i] = irBuffer[i+1];
    }
    redBuffer[BUFFER_SIZE-1] = redValue;
    irBuffer[BUFFER_SIZE-1] = irValue;

    maxim_heart_rate_and_oxygen_saturation(irBuffer, BUFFER_SIZE, redBuffer, 
                                          &spo2, &validSPO2, 
                                          &maximHeartRate, &validHeartRate);
    
    if (millis() - lastDisplayTime > displayInterval) {
      displayReadingScreen();
      lastDisplayTime = millis();
    }
    
  } else {

    if (fingerWasPresent) {
      maximHeartRate = 0; 
      spo2 = 0;
      fingerWasPresent = false;
      
      displayWaitingScreen();
      lastDisplayTime = millis(); 
    }
    
    if (millis() - lastDisplayTime > displayInterval) {
      displayWaitingScreen();
      lastDisplayTime = millis();
    }
  }

  yield(); 
}

void initializeLCD() {
  byte heart[8] = {
    0b00000,
    0b01010,
    0b11111,
    0b11111,
    0b11111,
    0b01110,
    0b00100,
    0b00000
  };

  lcd.init();
  lcd.backlight(); 
  lcd.clear();
  lcd.createChar(0, heart); 
  lcd.print("System Booting...");
  delay(1000);
}

void initializeSensor() {
  if (particleSensor.begin(Wire, I2C_SPEED_STANDARD) == false) {
    Serial.println("MAX3010x was not found. Please check wiring/address.");
    lcd.clear();
    lcd.print("SENSOR ERROR!");
    for(;;);
  }
  
  byte ledBrightness = 60; 
  byte sampleAverage = 4; 
  byte ledMode = 2; 
  byte sampleRate = 100; 
  int pulseWidth = 411; 
  int adcRange = 4096; 
  
  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); 

  particleSensor.setPulseAmplitudeRed(0x2A); 
  particleSensor.setPulseAmplitudeIR(0x2A); 
  
  particleSensor.clearFIFO(); 
  Serial.println("Sensor initialized.");
}

void displayWaitingScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place finger");
  lcd.setCursor(0, 1);
  lcd.print("to start reading.");
}

void displayReadingScreen() {
  lcd.clear();
  
  lcd.setCursor(0, 0);
  lcd.print("HR:");
  
  const int MAX_PLAUSIBLE_HR = 150; 
  
  if (maximHeartRate > 20 && maximHeartRate <= MAX_PLAUSIBLE_HR && validHeartRate == 1) {
    lcd.print(maximHeartRate);
    lcd.setCursor(7, 0);
    lcd.write(byte(0)); 
  } else {
    lcd.print("---"); 
  }
  lcd.print(" BPM");
  
  lcd.setCursor(0, 1);
  lcd.print("SpO2:");
  
  if (spo2 > 0 && validSPO2 == 1) {
    lcd.print(spo2);
  } else {
    lcd.print("---"); 
  }
  lcd.print(" %");
  
  if (validSPO2 == 1) {
    lcd.setCursor(14, 1);
    lcd.print("OK");
  }
}
