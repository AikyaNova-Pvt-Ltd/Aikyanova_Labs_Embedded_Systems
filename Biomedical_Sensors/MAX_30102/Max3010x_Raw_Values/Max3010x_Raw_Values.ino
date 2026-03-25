 /*
 * ------------------------------------------------------------------------
 * AikyaNova Labs Embedded Systems
 * ------------------------------------------------------------------------
 * Developed by: AikyaNova Labs
 * Firmware: This firmware reads raw Photoplethysmography (PPG) Infrared (IR) and Red
 *           light data using the MAX3010x optical sensor and displays the numerical
 *           values in real-time on a 0.96" OLED display and the Serial Monitor.
 *           This is Sketch 1 of 4 in the AikyaNova Pulse Oximeter series.
 *           Use this sketch first to verify your hardware is connected correctly
 *           before moving on to the plotting and BPM sketches.
 * Copyright (c) 2025 AikyaNova™
 * Licensed under the AikyaNova Non-Commercial License.
 * * Dependencies & Credits:
 * - SparkFun MAX3010x Sensor Library (BSD License)
 * - Adafruit GFX and SSD1306 Libraries by Adafruit Industries (BSD License)
 * ------------------------------------------------------------------------
 */

#include <Wire.h>             // I2C communication library (used by both sensor and OLED)
#include "MAX30105.h"         // SparkFun MAX3010x optical sensor library
#include <Adafruit_GFX.h>     // Core graphics library for the OLED display
#include <Adafruit_SSD1306.h> // SSD1306 OLED display driver

// ========================================================================
// HARDWARE & DISPLAY SETTINGS
// ========================================================================
#define SCREEN_WIDTH   128  // OLED display width, in pixels
#define SCREEN_HEIGHT  64   // OLED display height, in pixels
#define OLED_RESET     -1   // -1 = OLED RST pin not wired to a GPIO; reset handled by hardware power-on
#define SCREEN_ADDRESS 0x3C // Default I2C address for most 0.96" SSD1306 OLEDs; change to 0x3D if not found

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ========================================================================
// SENSOR OBJECT
// ========================================================================
MAX30105 particleSensor; // MAX30105 is fully compatible with the MAX30102 sensor

// ========================================================================
// SCROLL TEXT HELPER
// ========================================================================
// Scrolls a single line of text right-to-left across the vertical centre of
// the display. Uses textSize 2 (12px wide × 16px tall per character).
// setTextWrap(false) prevents the text from breaking onto a second line.
//
// Parameters:
//   text        — the string to scroll
//   numPasses   — how many times the full string crosses the screen
//   drainSensor — set true ONLY after particleSensor.begin() has been called.
//                 When true, the IR value is sampled every animation frame.
//                 If a finger is detected (IR >= 15000) the scroll aborts
//                 immediately and returns true, so the caller can react at once.
//
// Returns: true if aborted by finger detection, false if completed normally.
bool scrollText(const char* text, int numPasses, bool drainSensor = false) {
  int charPxWidth = 6 * 2;                     // 12px per character at textSize 2
  int textPxWidth = strlen(text) * charPxWidth;
  int yPos        = (SCREEN_HEIGHT - 16) / 2;  // vertically centred (16px = textSize 2 height)

  for (int pass = 0; pass < numPasses; pass++) {
    // x starts at the right edge and moves left until the text is fully off-screen
    for (int x = SCREEN_WIDTH; x > -textPxWidth; x -= 3) {
      display.clearDisplay();
      display.setTextWrap(false);   // prevent text wrapping to the next line
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(x, yPos);
      display.print(text);
      display.display();
      if (drainSensor && particleSensor.getIR() >= 15000) return true; // finger detected — abort immediately
      delay(8);
    }
  }
  return false;
}

// ========================================================================
// SETUP
// ========================================================================
void setup() {
  Serial.begin(115200); // Open Serial at 115200 baud — match this in Tools > Serial Monitor

  delay(1000); // Allow the Serial Monitor time to connect before printing

  // --- CROSS-PLATFORM I2C INITIALIZATION ---
  // The ESP32 and ESP8266 use different GPIO pins for I2C than standard Arduino boards.
  // This block detects which board is being compiled for and sets the correct pins.
#if defined(ESP32)
  Wire.begin(21, 22); // ESP32: SDA = GPIO 21, SCL = GPIO 22
#elif defined(ESP8266)
  Wire.begin(4, 5);   // ESP8266: SDA = D2 (GPIO 4), SCL = D1 (GPIO 5)
#else
  Wire.begin();       // Arduino Uno / Nano / Mega: uses default hardware I2C pins (A4/A5)
#endif

  // --- OLED DISPLAY INITIALIZATION ---
  // SSD1306_SWITCHCAPVCC tells the library to generate the display voltage internally.
  // If begin() returns false, the I2C address is wrong or wiring is incorrect.
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed. Check wiring."));
    while (1) delay(10); // Halt — cannot continue without a display
  }

  // --- WELCOME SCREEN ---
  // Scrolls the product name once across the display before sensor init.
  // drainSensor=false because particleSensor.begin() has not been called yet.
  scrollText("Welcome to AikyaNova Pulse Oximeter Demo Board", 1);

  // --- SENSOR INITIALIZATION ---
  // I2C_SPEED_FAST runs the bus at 400 kHz instead of the default 100 kHz.
  // This is required to read sensor data fast enough without falling behind.
  Serial.println(F("Initializing MAX30102..."));
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println(F("MAX30102 not found. Check wiring / power / I2C address."));
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 10);
    display.println(F("Sensor Error!"));
    display.display();
    while (1) delay(10); // Halt — cannot continue without a sensor
  }
  Serial.println(F("Sensor found!"));

  // --- SENSOR CONFIGURATION ---
  // sensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange)
  //
  // ledBrightness : LED current 0–255 (0 = off, 255 ≈ 50mA). 60 ≈ 12mA — good starting point.
  // sampleAverage : How many raw ADC samples are averaged into one FIFO entry.
  //                 Higher = smoother signal, lower temporal resolution.
  //                 Valid: 1, 2, 4, 8, 16, 32
  // ledMode       : 1 = Red only | 2 = Red + IR | 3 = Red + IR + Green (MAX30105 only)
  // sampleRate    : ADC samples per second before averaging.
  //                 Valid: 50, 100, 200, 400, 800, 1000, 1600, 3200
  //                 Effective output rate = sampleRate / sampleAverage
  // pulseWidth    : LED on-time in microseconds. Longer = more light collected = better SNR.
  //                 69 = 15-bit | 118 = 16-bit | 215 = 17-bit | 411 = 18-bit ADC resolution
  // adcRange      : Full-scale ADC range. Smaller = more sensitive; larger = wider range.
  //                 Valid: 2048, 4096, 8192, 16384
  byte ledBrightness = 60;
  byte sampleAverage = 4;
  byte ledMode       = 2;   // Red + IR
  int  sampleRate    = 100;
  int  pulseWidth    = 411; // 18-bit ADC resolution — best signal quality
  int  adcRange      = 4096;

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
  Serial.println(F("Setup done. Waiting for finger..."));

  // --- WAIT UNTIL FINGER IS PLACED ---
  // An IR reading above 15,000 reliably indicates a finger is covering the sensor.
  // The scrollText call animates the OLED while waiting and aborts the moment
  // a finger is detected, so there is no delay between placement and first reading.
  while (particleSensor.getIR() < 15000) {
    if (scrollText("Place your finger on sensor", 1, true)) break; // finger detected mid-scroll — exit immediately
  }

  Serial.println(F("Finger detected. Starting readings."));
}

// ========================================================================
// MAIN LOOP
// ========================================================================
// What to observe:
//   IR  value — rises significantly when a finger is placed (typically 50,000–250,000).
//               You can also see a small pulsatile variation with each heartbeat.
//   RED value — similar to IR but lower in amplitude. The ratio RED/IR is what the
//               SpO2 sketch (Sketch 4) uses to estimate blood oxygen saturation.
void loop() {
  long irValue  = particleSensor.getIR();
  long redValue = particleSensor.getRed();

  // --- FINGER REMOVED CHECK ---
  // If IR drops below 15,000 the sensor is reading open air — display prompt and wait.
  // Scrolling continues until the finger is placed again; returns to loop() immediately after.
  if (irValue < 15000) {
    Serial.println(F("No finger detected."));
    while (true) {
      if (scrollText("Place your finger on sensor", 1, true)) break; // finger detected mid-scroll
      if (particleSensor.getIR() >= 15000) break;                    // finger detected between passes
    }
    return; // return to loop() — next iteration will immediately read valid IR/RED values
  }

  // --- SERIAL MONITOR OUTPUT ---
  // Open Tools > Serial Monitor @ 115200 baud to view these values.
  // IR and RED are raw 18-bit ADC counts (0–262143 at adcRange=4096).
  Serial.print(F("IR="));
  Serial.print(irValue);
  Serial.print(F("\tRED="));
  Serial.println(redValue);

  // --- OLED DISPLAY ---
  // Shows both raw values in real time. textSize 2 = 12px wide × 16px tall per character.
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  // IR value — top half of screen
  display.setCursor(0, 0);
  display.print(F("IR: "));
  display.print(irValue);

  // RED value — bottom half of screen
  display.setCursor(0, 36);
  display.print(F("RED:"));
  display.print(redValue);

  display.display();

  delay(20); // ~50 Hz refresh rate — fast enough to see beat-to-beat variation
}
