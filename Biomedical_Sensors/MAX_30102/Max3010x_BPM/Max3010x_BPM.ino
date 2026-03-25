 /*
 * ------------------------------------------------------------------------
 * AikyaNova Labs Embedded Systems
 * ------------------------------------------------------------------------
 * Developed by: AikyaNova Labs
 * Firmware: Reads raw IR data from the MAX30105/102 sensor, applies a
 *           dynamic threshold peak-detection algorithm to measure heart
 *           rate, and automatically scales LED brightness to compensate
 *           for different skin tones (melanin levels). Displays live BPM
 *           on the OLED and outputs the PPG waveform to the Serial Plotter.
 *           This is Sketch 3 of 4 in the AikyaNova Pulse Oximeter series.
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
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1     // -1 = OLED RST pin not wired to a GPIO; reset handled by hardware power-on
#define SCREEN_ADDRESS 0x3C   // Default I2C address for most 0.96" SSD1306 OLEDs; change to 0x3D if not found

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
uint8_t  beatCount  = 0;           // Counts confirmed beats; BPM is not calculated until the 2nd beat.
                                   // This prevents the first interval (lastBeatMs set arbitrarily in
                                   // setup/finger-return, not at a real beat) from seeding a wrong BPM.
float bpm = 0;                     // Calculated Beats Per Minute

float env = 0;                     // Envelope tracker (calculates the amplitude of the pulse wave)
const float envAlpha = 0.95;       // Smoothing factor for the envelope (0.95 = 95% past, 5% new)

// REFRACTORY_MS is the minimum time allowed between valid beats.
// 400ms limits the maximum detectable heart rate to 150 BPM, which is sufficient for
// a resting/light-activity consumer device. Increased from 300ms to 400ms specifically
// to block the dicrotic notch on fair skin tones — the notch typically arrives
// 300-350ms after the main systolic peak and was being counted as a second beat,
// causing doubled BPM readings (~140 BPM instead of ~70 BPM) on fair skin.
const uint32_t REFRACTORY_MS = 400;

uint32_t heartBeatTimer = 0;       // Timer to control how long the OLED heart icon stays visible
uint32_t lastBeatDetectedMs = 0;   // Timestamp of last confirmed beat, used for stale BPM timeout
const uint32_t BPM_TIMEOUT_MS = 3000; // Reset BPM display if no beat detected for 3 seconds

bool warmupDone = false;           // Flag to skip finger-detection check during LED auto-brightness warmup
uint32_t warmupStart = 0;          // Timestamp when finger was first placed
const uint32_t WARMUP_MS = 500;    // Allow 500ms for auto-brightness to stabilize before applying finger threshold

// Signal quality gate: the envelope (average AC amplitude after /8 scaling) must exceed
// this value before any beat is accepted. Below it the signal is too weak or too noisy
// to trust — caused by a barely-touching finger, motion artifact, or poor skin contact.
// Tune upward if you see spurious BPM readings; tune downward if detection is too slow to start.
const float MIN_SIGNAL_QUALITY = 8.0f;

// ========================================================================
// HEART SYMBOL HELPER
// ========================================================================
// Draws a filled 13×11 px heart at (x, y) = top-left of its bounding box.
// Built from two filled circles (top lobes) + a filled triangle (bottom point).
//
//   ● ●         ← two circles, r=3, centres at (x+3,y+3) and (x+9,y+3)
//  █████████
//   ███████
//    █████
//     ███
//      █        ← triangle apex at (x+6, y+10)
//
void drawHeart(int16_t x, int16_t y, uint16_t color) {
  display.fillCircle(x + 3,  y + 3, 3, color); // left lobe
  display.fillCircle(x + 9,  y + 3, 3, color); // right lobe
  display.fillTriangle(x, y + 4, x + 12, y + 4, x + 6, y + 10, color); // lower point
}

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
//   drainSensor — set true ONLY after sensor.begin() has been called.
//                 When true, the IR value is sampled every animation frame.
//                 If a finger is detected (IR >= 20000) the scroll aborts
//                 immediately and returns true, so the caller can react at once.
//
// Returns: true if aborted by finger detection, false if completed normally.
bool scrollText(const char* text, int numPasses, bool drainSensor = false) {
  int charPxWidth = 6 * 2;
  int textPxWidth = strlen(text) * charPxWidth;
  int yPos        = (SCREEN_HEIGHT - 16) / 2;  // vertically centred (16px = textSize 2 height)

  for (int pass = 0; pass < numPasses; pass++) {
    for (int x = SCREEN_WIDTH; x > -textPxWidth; x -= 3) {
      display.clearDisplay();
      display.setTextWrap(false);
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(x, yPos);
      display.print(text);
      display.display();
      if (drainSensor && sensor.getIR() >= 20000) return true; // finger detected — abort immediately
      delay(8);
    }
  }
  return false;
}

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
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed. Check wiring!"));
    while (1) delay(10);
  }

  // --- WELCOME SCREEN ---
  // Scrolls the product name once across the display before sensor init.
  // drainSensor=false because sensor.begin() has not been called yet.
  scrollText("Welcome to AikyaNova Pulse Oximeter Demo Board", 1);

  // --- SENSOR INITIALIZATION ---
  // I2C_SPEED_FAST (400kHz) is required to pull data fast enough for smooth waveforms
  if (!sensor.begin(Wire, I2C_SPEED_FAST)) {
    display.clearDisplay();
    display.setTextWrap(true);
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 10);
    display.println(F("Sensor Error!"));
    display.display();
    Serial.println(F("Sensor not found. Check wiring!"));
    while (1) delay(10);
  }

  // --- SENSOR CONFIGURATION ---
  // setup(powerLevel, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange)
  // We use a 411us pulse width here. This provides the highest possible ADC resolution (18-bit),
  // which is absolutely critical for resolving tiny AC pulse waves through dark skin tones.
  sensor.setup(0x00, 8, 2, 400, 411, 4096);

  // Set initial LED amplitudes
  sensor.setPulseAmplitudeIR(currentPower);
  sensor.setPulseAmplitudeRed(15); // Red is kept low as this specific algorithm primarily relies on IR

  // --- WAIT UNTIL FINGER IS PLACED ---
  Serial.println(F("Waiting for finger..."));
  while (sensor.getIR() < 20000) {
    if (scrollText("Place your finger on sensor", 1, true)) break; // finger detected mid-scroll — exit immediately
  }

  // Seed variables after finger is confirmed
  delay(200);
  dcIR       = sensor.getIR();
  lastBeatMs = millis();
  lastBeatDetectedMs = millis();
  beatCount  = 0; // reset so first interval is skipped
  Serial.println(F("Finger detected. Starting BPM readings."));
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
  // We skip this check during the warmup window to give auto-brightness time to stabilize,
  // which prevents false "no finger" detections on dark skin tones at startup.
  if (ir < 20000 && warmupDone) {
    // Reset math variables so BPM doesn't spike artificially when finger is replaced
    bpm = 0;
    dcIR = ir;
    warmupDone = false; // Require a fresh warmup next time finger is placed
    env = 0;            // Reset envelope so threshold doesn't trigger on stale amplitude

    // Reset LED power back to a safe baseline when finger is removed
    if (currentPower != 40) {
      currentPower = 40;
      sensor.setPulseAmplitudeIR(currentPower);
    }

    // Scroll finger prompt — abort as soon as finger is placed again
    while (true) {
      if (scrollText("Place your finger on sensor", 1, true)) break; // finger detected mid-scroll
      if (sensor.getIR() >= 20000) break;                            // finger detected between passes
    }

    // Re-seed baseline cleanly after finger is replaced
    delay(200);
    dcIR       = sensor.getIR();
    lastBeatMs = millis();
    lastBeatDetectedMs = millis();
    beatCount  = 0; // reset so first interval after re-placement is skipped
    return;
  }

  // Start warmup timer once a finger is detected
  if (!warmupDone) {
    if (warmupStart == 0) warmupStart = millis();
    if (millis() - warmupStart >= WARMUP_MS) {
      warmupDone = true;
      warmupStart = 0;
    }
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
    else if (ir > 220000 && currentPower > 5) {
      // Signal is heavily saturated (very fair skin). Decrease aggressively to
      // prevent a large AC amplitude that makes the dicrotic notch cross the threshold.
      currentPower = (currentPower > 10) ? currentPower - 5 : 5;
      sensor.setPulseAmplitudeIR(currentPower);
    }
    else if (ir > 180000 && currentPower > 5) {
      // Signal is mildly strong. Gentle decrease to stay in optimal range.
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
  int32_t x = ac / 8; // Scale down the signal to fit comfortably on serial plotters (assumes adcRange=4096)

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
  // AND the peak height is above the dynamic noise threshold
  // AND the signal envelope is above the minimum quality gate.
  // The quality gate rejects beats during motion artifact, barely-touching finger,
  // or when the auto-brightness hasn't yet settled on the correct LED power.
  bool isPeak = (prev > prev2) && (prev > x) && (prev > thresh) && (env >= MIN_SIGNAL_QUALITY);
  bool beatFired = false;

  if (isPeak) {
    uint32_t now = millis();
    uint32_t dt  = now - lastBeatMs; // Time elapsed since last beat in milliseconds

    // Validate the beat against the human refractory period (preventing double-counting)
    if (dt > REFRACTORY_MS) {
      beatCount++;
      lastBeatMs         = now;
      lastBeatDetectedMs = now;
      beatFired          = true; // triggers OLED heart and Serial spike regardless of BPM calc

      if (beatCount == 1) {
        // First beat after placement: lastBeatMs was set arbitrarily (not at a real beat),
        // so dt is meaningless. Record the timestamp but skip BPM calculation entirely.
        // BPM will be computed cleanly from the second beat onward.
      } else {
        float instBpm = 60000.0f / dt; // Convert ms between real beats to BPM

        if (bpm == 0) {
          bpm = instBpm; // Seed with first valid interval (beat 2)
        } else if (instBpm > bpm * 1.4f) {
          // Plausibility check: sudden >40% jump is likely a dicrotic notch or artifact
          // that slipped past the refractory gate — apply near-zero weight.
          bpm = 0.98f * bpm + 0.02f * instBpm;
        } else {
          // Normal smoothing: 30% new beat weight.
          // Responds to real HR changes within ~3 beats (~2.5s at 70 BPM).
          bpm = 0.70f * bpm + 0.30f * instBpm;
        }
      }
    }
  }

  // Stale BPM timeout: if no beat detected for BPM_TIMEOUT_MS, clear the reading
  if (bpm > 0 && (millis() - lastBeatDetectedMs > BPM_TIMEOUT_MS)) {
    bpm = 0;
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
  display.clearDisplay();
  display.setTextWrap(false);

  // Draw "BPM:" label on the top line (4 chars × 12px = 48px — fits without overflow)
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 5);
  display.print("BPM:");

  // Draw the calculated BPM value on the bottom line (large, centre-aligned)
  display.setTextSize(3);
  if (bpm > 20 && bpm < 250) {
    // Centre-align: each char is 18px wide at textSize 3
    int bpmVal = (int)bpm;
    int digits = (bpmVal >= 100) ? 3 : 2;
    int xPos = (SCREEN_WIDTH - digits * 18) / 2;
    display.setCursor(xPos, 32);
    display.print(bpmVal);
  } else if (env < MIN_SIGNAL_QUALITY) {
    // Signal quality too low — finger barely touching or motion artifact
    display.setTextSize(1);
    display.setCursor(22, 38);
    display.print("LOW SIGNAL");
    display.setCursor(10, 50);
    display.print("Adjust finger");
  } else {
    display.setCursor(46, 32);
    display.print("--"); // Signal OK but still calculating first beat
  }

  // Handle the blinking heart indicator
  if (beatFired) {
    heartBeatTimer = millis() + 150; // Keep heart visible for 150ms after a beat
  }

  // Draw the heart symbol if the beat timer is still active
  // Positioned top-right: bounding box x=113, y=1 → occupies px 113-126 × 1-12
  if (millis() < heartBeatTimer) {
    drawHeart(113, 1, SSD1306_WHITE);
  }

  display.display(); // Push the rendered buffer to the physical screen
}
