#include <Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <Adafruit_MLX90614.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

const char* ssid = "";
const char* password = "";
const char* host = ""; 
const uint16_t port = 5050;

WiFiClient client;

MAX30105 particleSensor;
Adafruit_MLX90614 mlx;

uint32_t irBuffer[100], redBuffer[100];
int32_t spo2, heartRate;
int8_t validSPO2, validHeartRate;

void setup() {
  Serial.begin(115200);
  delay(250);

  Wire.begin();
  Wire.setClock(100000);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected");

  if (!client.connect(host, port)) {
    Serial.println("Failed to connect to MATLAB server");
    while (1);
  }
  Serial.println("Connected to MATLAB");

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
      break;
    } else {
      Serial.println("No finger detected.");
      delay(500);
    }
  }

  while (true) {
    long irSum = 0;
    for (int i = 0; i < 10; i++) {
      while (!particleSensor.available()) particleSensor.check();
      irSum += particleSensor.getIR();
      particleSensor.nextSample();
    }

    long avgIR = irSum / 10;
    if (avgIR < 10000) {
      Serial.println("Finger removed.");
      delay(250);
      return;
    }

    for (int i = 0; i < 100; i++) {
      while (!particleSensor.available()) particleSensor.check();
      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample();
    }

    maxim_heart_rate_and_oxygen_saturation(irBuffer, 100, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
    float temp = mlx.readObjectTempC();

    String data = "SPO2=";
    data += (validSPO2 ? String(spo2) : "Invalid");
    data += ",TEMP=";
    data += String(temp, 2);
    data += "\n";

    Serial.print("Sent: "); Serial.print(data);

    if (client.connected()) {
      client.print(data);
    } else {
      Serial.println("Disconnected. Reconnecting...");
      client.stop();
      client.connect(host, port);
    }

    delay(250);
  }
}
