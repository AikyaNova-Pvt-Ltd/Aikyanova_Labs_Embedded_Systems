 /*
 * ------------------------------------------------------------------------
 * AikyaNova Labs Embedded Systems
 * ------------------------------------------------------------------------
 * Developed by: AikyaNova Labs
 * Firmware: Reads raw IR data from MAX30105/102, applies a dynamic 
 *           threshold algorithm to detect heartbeats, automatically scales
 *           sensor brightness for different skin tones (melanin levels), 
 *           and outputs BPM to an I2C OLED and Serial Plotter.
 * Copyright (c) 2025 AikyaNova™
 * Licensed under the AikyaNova Non-Commercial License.
 * * Dependencies & Credits:
 * - SparkFun MAX3010x Sensor Library (BSD License)
 * - PBA & SpO2 Algorithms by Maxim Integrated Products (MIT-style License)
 * - Adafruit GFX and SSD1306 Libraries by Adafruit Industries (BSD License)
 * ------------------------------------------------------------------------
 */

#include <Wire.h>
#include "MAX30105.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ========================================================================
// HARDWARE & DISPLAY SETTINGS
// ========================================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MAX30105 sensor;

// ========================================================================
// AUTO-LED BRIGHTNESS CONTROL VARIABLES
// ========================================================================
// currentPower controls the current injected into the IR LED (0-255).
// 40 is a safe starting baseline (~8mA) that prevents blinding the ADC initially.
byte currentPower = 40;            
unsigned long lastAdjustTime = 0;  // Timer to prevent adjusting power too rapidly

// ========================================================================
// SIGNAL PROCESSING & BPM VARIABLES
// ========================================================================
int32_t dcIR = 0;                  // Tracks the slow-moving DC baseline (tissue/venous blood)
const int SHIFT_DC = 4;            // Bitshift for DC filter (equivalent to division by 16 for speed)

int32_t prev = 0;                  // Previous AC signal data point (t-1)
int32_t prev2 = 0;                 // Previous AC signal data point (t-2)

uint32_t lastBeatMs = 0;           // Timestamp of the last detected pulse
float bpm = 0;                     // Calculated Beats Per Minute

float env = 0;                     // Envelope tracker (calculates the amplitude of the pulse wave)
const float envAlpha = 0.95;       // Smoothing factor for the envelope (0.95 = 95% past, 5% new)

// REFRACTORY_MS is the minimum time allowed between valid beats. 
// 300ms limits the maximum detectable heart rate to 200 BPM. 
// This prevents the algorithm from falsely counting the "dicrotic notch" as a second beat.
const uint32_t REFRACTORY_MS = 300; 

uint32_t heartBeatTimer = 0;       // Timer to control how long the OLED heart icon stays visible

// ========================================================================
// SETUP
// ========================================================================
void setup() {
  Serial.begin(115200);

  // --- CROSS-PLATFORM I2C INITIALIZATION ---
  // ESP32 and ESP8266 often use different default I2C pins than standard Arduinos
#if defined(ESP32)
  Wire.begin(21, 22); // SDA, SCL for ESP32
#elif defined(ESP8266)
  Wire.begin(4, 5);   // SDA, SCL for ESP8266
#else
  Wire.begin();       // Default for AVR (Uno/Nano)
#endif

  // --- OLED INITIALIZATION ---
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed. Check wiring!"));
    while(1) delay(10); // Halt execution if screen fails
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 25);
  display.print("Initializing...");
  display.display();

  // --- SENSOR INITIALIZATION ---
  // I2C_SPEED_FAST (400kHz) is required to pull data fast enough for smooth waveforms
  if (!sensor.begin(Wire, I2C_SPEED_FAST)) {
    display.clearDisplay();
    display.setCursor(15, 25);
    display.print("Sensor not found!");
    display.display();
    while (1) delay(10); // Halt execution if sensor fails
  }

  // --- SENSOR CONFIGURATION ---
  // setup(powerLevel, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange)
  // We use a 411us pulse width here. This provides the highest possible ADC resolution (18-bit),
  // which is absolutely critical for resolving tiny AC pulse waves through dark skin tones.
  sensor.setup(0x00, 8, 2, 400, 411, 4096);
  
  // Set initial LED amplitudes
  sensor.setPulseAmplitudeIR(currentPower); 
  sensor.setPulseAmplitudeRed(15); // Red is kept low as this specific algorithm primarily relies on IR

  delay(200); // Give the sensor time to take initial readings
  dcIR = sensor.getIR(); // Seed the initial DC baseline
  lastBeatMs = millis();
}

// ========================================================================
// MAIN LOOP
// ========================================================================
void loop() {
  // Fetch the latest raw Infrared reading
  int32_t ir = sensor.getIR();

  // ---------------------------------------------------------
  // PHASE 1: "Place Finger" Status Warning
  // ---------------------------------------------------------
  // 20,000 is our "open air" threshold. If reading is below this, no finger is present.
  if (ir < 20000) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 18);
    display.print("   Place");
    display.setCursor(30, 34);
    display.print("Finger");
    display.display();
    
    // Reset math variables so BPM doesn't spike artificially when finger is replaced
    bpm = 0;
    dcIR = ir; 
    
    // Reset LED power back to a safe baseline when finger is removed
    if (currentPower != 40) {
      currentPower = 40;
      sensor.setPulseAmplitudeIR(currentPower);
    }
    
    delay(50); // Small delay to prevent OLED flickering
    return;    // Exit the loop early—no need to run signal processing on open air
  }

  // ---------------------------------------------------------
  // PHASE 2: Auto-LED Brightness Feedback Loop
  // ---------------------------------------------------------
  // This compensates for varying melanin levels and tissue thickness.
  // We only check every 50ms so the ADC has time to register the previous power change.
  if (millis() - lastAdjustTime > 50) {
    if (ir < 60000 && currentPower < 250) {
      // Signal is too weak (dark skin / thick finger). Increment LED power.
      currentPower += 2;
      sensor.setPulseAmplitudeIR(currentPower);
    } 
    else if (ir > 200000 && currentPower > 5) {
      // Signal is too strong (fair skin). ADC is risking saturation. Decrement LED power.
      currentPower -= 2;
      sensor.setPulseAmplitudeIR(currentPower);
    }
    lastAdjustTime = millis();
  }

  // ---------------------------------------------------------
  // PHASE 3: Signal Processing & DC Removal
  // ---------------------------------------------------------
  // Update the slow-moving DC average using a simple low-pass IIR filter
  dcIR = dcIR + ((ir - dcIR) >> SHIFT_DC);
  
  // Isolate the AC component (the actual pulse wave) by subtracting the DC baseline
  int32_t ac = dcIR - ir;
  int32_t x = ac / 8; // Scale down the signal to fit comfortably on serial plotters

  // ---------------------------------------------------------
  // PHASE 4: Dynamic Thresholding (Envelope Tracking)
  // ---------------------------------------------------------
  // Get absolute value of the signal
  float absx = (x >= 0) ? x : -x;
  
  // Update the envelope (tracks the average peak amplitude of the wave)
  env = envAlpha * env + (1.0 - envAlpha) * absx;
  
  // Our trigger threshold is dynamically set to 50% of the current wave amplitude.
  // This allows the algorithm to adapt perfectly to both weak and strong pulses.
  float thresh = 0.5f * env;

  // ---------------------------------------------------------
  // PHASE 5: Peak Detection & BPM Calculation
  // ---------------------------------------------------------
  // A peak is detected if the wave has changed direction (prev > prev2 and prev > current) 
  // AND the peak height is above the noise threshold.
  bool isPeak = (prev > prev2) && (prev > x) && (prev > thresh);
  bool beatFired = false;

  if (isPeak) {
    uint32_t now = millis();
    uint32_t dt = now - lastBeatMs; // Time elapsed since last beat in milliseconds
    
    // Validate the beat against the human refractory period (preventing double-counting)
    if (dt > REFRACTORY_MS) {
      float instBpm = 60000.0f / dt; // Convert milliseconds between beats to Beats Per Minute
      
      // If this is the first beat, seed the BPM immediately. Otherwise, apply a low-pass filter.
      if (bpm == 0) bpm = instBpm;
      bpm = 0.85f * bpm + 0.15f * instBpm; // Smooth out rapid BPM fluctuations
      
      lastBeatMs = now;
      beatFired = true; // Flag used to trigger visuals (Serial spike and OLED heart)
    }
  }

  // Shift data points back in time for the next loop iteration
  prev2 = prev;
  prev = x;

  // ---------------------------------------------------------
  // PHASE 6: Serial Plotter Output
  // ---------------------------------------------------------
  // Open Tools > Serial Plotter @ 115200 baud to view these waveforms.
  Serial.print("Signal:"); 
  Serial.print(x);
  Serial.print(", Threshold:"); 
  Serial.print(thresh);
  Serial.print(", BeatMarker:"); 
  // Draw a sharp visual spike up to the envelope height when a beat is confirmed
  Serial.println(beatFired ? env : 0); 

  // ---------------------------------------------------------
  // PHASE 7: OLED Display Update
  // ---------------------------------------------------------
  display.clearDisplay(); // Erase previous frame

  // Draw static "BPM:" label
  display.setTextSize(3);
  display.setCursor(10, 22);
  display.print("BPM:");

  // Draw the calculated BPM value
  display.setCursor(85, 22);
  if (bpm > 20 && bpm < 250) {
    display.print((int)bpm); // Cast float to int for cleaner display
  } else {
    display.print("--");     // Show dashes if BPM is out of human bounds or calculating
  }

  // Handle the blinking heart indicator
  if (beatFired) {
    heartBeatTimer = millis() + 150; // Keep heart visible for 150ms after a beat
  }

  // Draw the heart (circle) if the timer is still active
  if (millis() < heartBeatTimer) {
    display.fillCircle(115, 8, 6, SSD1306_WHITE); // Placed in the top right corner
  }

  display.display(); // Push the rendered buffer to the physical screen
}
