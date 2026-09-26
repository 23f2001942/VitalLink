#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

MAX30105 particleSensor;

uint32_t irBuffer[100];
uint32_t redBuffer[100];

int32_t bufferLength = 100;
int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Wire.begin();  // Default I2C for XIAO ESP32C3 (D4/D5)

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found. Check wiring.");
    while (1);
  }

  Serial.println("MAX30102 initialized. Place your finger on the sensor...");

  particleSensor.setup(
    60, 4, 2, 100, 411, 4096
  );
}

void loop() {
  // Finger Detection
  long irSum = 0;
  for (int i = 0; i < 10; i++) {
    while (!particleSensor.available()) particleSensor.check();
    irSum += particleSensor.getIR();
    particleSensor.nextSample();
  }

  long avgIR = irSum / 10;
  if (avgIR < 10000) {
    Serial.println("No finger detected. Waiting...");
    delay(500);
    return;
  }

  Serial.println("Finger detected. Stabilizing...");
  delay(1000);

  // Collect 100 samples
  for (int i = 0; i < bufferLength; i++) {
    while (!particleSensor.available()) particleSensor.check();
    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample();
  }

  // SpO₂ Calculation
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  // === If SpO₂ invalid, asking user to try again ===
  if (validSPO2 == 0) {
    Serial.println("SpO₂ reading invalid. Please lift and re-place your finger.");
    delay(1000);
    return;  // Restart loop
  }

  //Continuous Monitoring
  while (true) {
    // Shift 90 old samples
    for (int i = 10; i < 100; i++) {
      redBuffer[i - 10] = redBuffer[i];
      irBuffer[i - 10] = irBuffer[i];
    }

    long irSegmentSum = 0;

    // Read 10 new samples
    for (int i = 90; i < 100; i++) {
      while (!particleSensor.available()) particleSensor.check();
      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      irSegmentSum += irBuffer[i];
      particleSensor.nextSample();

      Serial.print("IR="); Serial.print(irBuffer[i]);
      Serial.print(", RED="); Serial.print(redBuffer[i]);
      Serial.print(", SpO₂=");
      Serial.print(spo2);
      Serial.print(" %, Valid=");
      Serial.println(validSPO2);
    }

    long avgSegmentIR = irSegmentSum / 10;

    // Finger removed?
    if (avgSegmentIR < 10000) {
      Serial.println("Finger removed. Restarting...");
      delay(500);
      break;  // Exit continuous monitoring loop
    }

    // Recalculate SpO₂
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

    if (validSPO2 == 0) {
      Serial.println("SpO₂ invalid. Please reposition your finger.");
      delay(1000);
      break;
    }
  }
}
