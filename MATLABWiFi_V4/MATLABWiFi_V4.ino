#include <Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <Adafruit_MLX90614.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

// — Wi-Fi & MATLAB TCP server info —
const char* ssid = "";
const char* password = "";
const char* host = ""; 
const uint16_t port = 5050;

WiFiClient   client;
MAX30105     particleSensor;
Adafruit_MLX90614 mlx;

// Buffers & results
uint32_t irBuffer[100], redBuffer[100];
int32_t  spo2, heartRate;
int8_t   validSPO2, validHR;

// Reconnect helpers
void ensureWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    unsigned long t = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t < 10000) {
      delay(250);
    }
  }
}
void ensureTCP() {
  if (!client.connected()) {
    client.stop();
    client.connect(host, port);
  }
}

void setup() {
  Serial.begin(115200);
  delay(250);
  Wire.begin(); Wire.setClock(100000);

  // connect Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
  }
  // connect TCP
  client.connect(host, port);

  // init MAX30102
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 missing"); while (1);
  }
  particleSensor.setup(60, 4, 2, 100, 411, 4096);

  // init MLX90614
  if (!mlx.begin()) {
    Serial.println("MLX90614 missing"); while (1);
  }
}

void loop() {
  ensureWiFi(); ensureTCP();
  if (WiFi.status()!=WL_CONNECTED || !client.connected()) {
    delay(500); return;
  }

  // 1) IR–based finger detect (10-sample avg)
  long sum = 0;
  for (int i=0; i<10; i++) {
    while (!particleSensor.available()) particleSensor.check();
    sum += particleSensor.getIR();
    particleSensor.nextSample();
  }
  float avgIR = sum / 10.0f;

  String msg;
  if (avgIR < 10000) {
    // no finger
    msg = "STATUS=NoFinger\n";
    client.print(msg); Serial.print(msg);
    delay(500);
    return;
  }

  // 2) finger detected → stabilizing
  msg = "STATUS=Stabilizing\n";
  client.print(msg); Serial.print(msg);
  delay(750);

  // 3) collect 100 samples
  for (int i=0; i<100; i++) {
    while (!particleSensor.available()) particleSensor.check();
    redBuffer[i] = particleSensor.getRed();
    irBuffer[i]  = particleSensor.getIR();
    particleSensor.nextSample();
  }

  // 4) compute SpO₂
  maxim_heart_rate_and_oxygen_saturation(
    irBuffer, 100, redBuffer,
    &spo2, &validSPO2, &heartRate, &validHR
  );

  // 5) read temp
  float temp = mlx.readObjectTempC();

  // 6) send data
  msg = "STATUS=Data,"
        "SPO2=" + String(validSPO2 ? spo2 : -1) + ","
        "TEMP=" + String(temp, 2) + "\n";
  client.print(msg); Serial.print(msg);

  delay(250);
}