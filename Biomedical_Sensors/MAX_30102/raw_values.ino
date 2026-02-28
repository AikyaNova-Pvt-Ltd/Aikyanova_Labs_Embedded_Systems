#include <Wire.h>
#include "MAX30105.h"   // SparkFun MAX3010x library header

MAX30105 particleSensor;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(21, 22); // ESP32 SDA=21, SCL=22 (change if your board differs)

  Serial.println("Initializing MAX30102/MAX30105...");

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) { // 400kHz
    Serial.println("MAX30102 not found. Check wiring/power/I2C pins.");
    while (1) delay(10);
  }

  Serial.println("Sensor found!");

  // Basic settings for PPG (SpO2-style)
  // brightness (LED current): 0-255 (start moderate)
  byte ledBrightness = 60;
  byte sampleAverage = 4;
  byte ledMode = 2;        // 2 = RED + IR
  int sampleRate = 100;    // 50-400
  int pulseWidth = 411;    // 69,118,215,411 (wider = better SNR, more power)
  int adcRange = 4096;     // 2048,4096,8192,16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);

  // Optional: turn on internal die temp reading later; ignore now.
  Serial.println("Setup done. Place finger on sensor.");
}

void loop() {
  // The library keeps reading FIFO internally; these return latest samples.
  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();

  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print("\tRED=");
  Serial.println(redValue);

  delay(20); // ~50 Hz printing (not the sensor sample rate)
}
