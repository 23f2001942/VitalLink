#include <Wire.h>
#include <Adafruit_MLX90614.h>

// Create sensor object
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Wire.begin();  

  // Initialize MLX90614 sensor
  if (!mlx.begin()) {
    Serial.println("Error connecting to MLX90614 sensor. Check wiring!");
    while (1); // Stop here if sensor not found
  }

  Serial.println("MLX90614 sensor ready!");
}

void loop() {
  // Read temperatures
  float ambientTemp = mlx.readAmbientTempC();
  float objectTemp = mlx.readObjectTempC();

  // Print to Serial Monitor
  Serial.print("Ambient Temp: ");
  Serial.print(ambientTemp);
  Serial.print(" °C\t");

  Serial.print("Object Temp: ");
  Serial.print(objectTemp);
  Serial.println(" °C");

  delay(1000); // Update every second
}
