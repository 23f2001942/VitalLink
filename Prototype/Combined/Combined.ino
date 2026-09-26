#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

MAX30105 particleSensor;
Adafruit_MLX90614 mlx;

uint32_t irBuffer[100], redBuffer[100];
int32_t spo2, heartRate;
int8_t validSPO2, validHeartRate;

void setup() {
  Serial.begin(115200);
  delay(250);

  Wire.begin();
  Wire.setClock(100000);  // Shared I2C speed

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 not found.");
    while (1);
  }
  particleSensor.setup(60, 4, 2, 100, 411, 4096);
  Serial.println("MAX30102 initialized.");

  if (!mlx.begin()) {
    Serial.println("MLX90614 not found.");
    while (1);
  }
  Serial.println("MLX90614 initialized.");
}

void loop() {
  // Wait for finger
  while (true) {
    long irSum = 0;
    for (int i = 0; i < 10; i++) {
      while (!particleSensor.available()) particleSensor.check();
      irSum += particleSensor.getIR();
      particleSensor.nextSample();
    }
    long avgIR = irSum / 10;

    if (avgIR >= 10000) {
      Serial.println("Finger detected. Stabilizing...");
      delay(750);
      break;  // Exit loop and begin measurement
    } else {
      Serial.println("No finger detected. Please place finger.");
      delay(500);
    }
  }

  // Finger is present — start continuous reading loop
  while (true) {
    // Check if finger is still there
    long irSum = 0;
    for (int i = 0; i < 10; i++) {
      while (!particleSensor.available()) particleSensor.check();
      irSum += particleSensor.getIR();
      particleSensor.nextSample();
    }

    long avgIR = irSum / 10;
    if (avgIR < 10000) {
      Serial.println("Finger removed. Restarting detection...");
      delay(250);
      return;  // Exit to top of loop()
    }

    // Collect 100 samples
    for (int i = 0; i < 100; i++) {
      while (!particleSensor.available()) particleSensor.check();
      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample();
    }

    maxim_heart_rate_and_oxygen_saturation(irBuffer, 100, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

    float temp = mlx.readObjectTempC();

    Serial.print("SpO₂: ");
    Serial.print(validSPO2 ? String(spo2) : "Invalid");
    Serial.print(" % | Temp: ");
    Serial.print(temp);
    Serial.println(" °C");

    delay(250);
  }
}