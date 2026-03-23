 /*
 * ------------------------------------------------------------------------
 * AikyaNova Labs Embedded Systems
 * ------------------------------------------------------------------------
 * Developed by: AikyaNova Labs
 * Firmware: This firmware reads raw Photoplethysmography (PPG) Infrared (IR) and Red 
 *           light data using the MAX3010x optical sensor and displays the numerical 
 *           values in real-time on a 0.96" OLED display and the Serial Monitor.
 * Copyright (c) 2025 AikyaNova™
 * Licensed under the AikyaNova Non-Commercial License.
 * * Dependencies & Credits:
 * - SparkFun MAX3010x Sensor Library (BSD License)
 * - PBA & SpO2 Algorithms by Maxim Integrated Products (MIT-style License)
 * - Adafruit GFX and SSD1306 Libraries by Adafruit Industries (BSD License)
 * ------------------------------------------------------------------------
 */

#include <Wire.h>
#include "MAX30105.h"   // SparkFun MAX3010x library header
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED display settings
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32/128x64 usually

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MAX30105 particleSensor;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // --- CROSS-PLATFORM I2C INITIALIZATION ---
  // Detects the board type and starts the I2C bus accordingly
#if defined(ESP32)
  Wire.begin(21, 22); // ESP32 standard I2C pins (SDA=21, SCL=22)
#elif defined(ESP8266)
  Wire.begin(4, 5);   // ESP8266 standard I2C pins (SDA=D2/4, SCL=D1/5)
#else
  Wire.begin();       // Standard Arduino (Uno, Nano, Mega) uses default hardware pins
#endif

  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed. Check wiring."));
    while(1) delay(10); // Don't proceed, loop forever
  }
  
  // Show an initial message on the OLED
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println(F(" Starting \n MAX3010x"));
  display.display();
  delay(1000);

  Serial.println("Initializing MAX30102/MAX30105...");
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) { // 400kHz
    Serial.println("MAX30102 not found. Check wiring/power/I2C pins.");
    display.clearDisplay();
    display.setCursor(0, 10);
    display.println(F("Sensor Error!"));
    display.display();
    while (1) delay(10);
  }

  Serial.println("Sensor found!");

  // Basic settings for PPG (SpO2-style)
  byte ledBrightness = 60; // brightness (LED current): 0-255
  byte sampleAverage = 4;
  byte ledMode = 2;        // 2 = RED + IR
  int sampleRate = 100;    // 50-400
  int pulseWidth = 411;    // 69,118,215,411 (wider = better SNR, more power)
  int adcRange = 4096;     // 2048,4096,8192,16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);

  Serial.println("Setup done. Place finger on sensor.");
  display.clearDisplay();
  display.setTextSize(2); 
  display.setCursor(0, 10);
  display.println(F("   Place \n  finger"));
  display.println(F(" on sensor"));
  display.display();
  delay(2000);
}

void loop() {
  // Read the latest samples
  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();

  // Print to Serial Monitor
  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print("\tRED=");
  Serial.println(redValue);

  // Print to OLED Display
  display.clearDisplay();
  
  // Display IR Value
  display.setTextSize(2); 
  display.setCursor(0, 0);
  display.print("IR: ");
  //display.setCursor(0, 18);
  display.print(irValue);
  
  // Display RED Value
  display.setCursor(0, 36);
  display.print("RED:");
  //display.setCursor(0, 50);
  display.print(redValue);
  
  display.display();

  delay(20); // ~50 Hz printing
}
