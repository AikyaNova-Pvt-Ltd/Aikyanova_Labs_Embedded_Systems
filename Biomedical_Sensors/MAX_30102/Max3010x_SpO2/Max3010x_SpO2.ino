/*
 * ------------------------------------------------------------------------
 * AikyaNova Labs Embedded Systems
 * ------------------------------------------------------------------------
 * Developed by: AikyaNova Labs
 * Firmware: Reads raw Infrared (IR) and Red optical data from the MAX30105/102
 *           sensor and computes two vital signs simultaneously:
 *           - Heart Rate (BPM) using a dynamic threshold peak-detection algorithm.
 *           - Blood Oxygen Saturation (SpO2) using the Red/IR Ratio-of-Ratios method.
 *           Features independent auto-scaling of both LEDs to compensate for
 *           varying melanin levels and tissue thickness across skin tones.
 *           Displays both values live on the OLED and outputs the PPG waveform,
 *           threshold, BPM and SpO2 traces to the Arduino Serial Plotter.
 *           This is Sketch 4 of 4 in the AikyaNova Pulse Oximeter series.
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
// DUAL AUTO-LED BRIGHTNESS (MELANIN COMPENSATION)
// ========================================================================
// Melanin absorbs visible Red light significantly more than Infrared (IR) light.
// To ensure a high SNR for accurate SpO2 on all skin tones, both LEDs are
// independently adjusted dynamically.
byte irPower  = 40;           // Starting power for IR LED (~8mA)
byte redPower = 40;           // Starting power for Red LED (~8mA)
unsigned long lastAdjustTime = 0;

// ========================================================================
// DC REMOVAL & BASELINE TRACKING
// ========================================================================
int32_t dcIR  = 0;
int32_t dcRED = 0;
const int SHIFT_DC = 4;       // EMA filter: >> 4 ≡ divide by 16

// ========================================================================
// PEAK DETECTION & BPM VARIABLES
// ========================================================================
int32_t prev = 0, prev2 = 0;
uint32_t lastBeatMs        = 0;
uint8_t  beatCount         = 0;   // BPM not calculated until 2nd beat to avoid
                                   // a bad first interval from the arbitrary lastBeatMs seed.
float bpm = 0;

float env = 0;
const float envAlpha = 0.95f;

// 400ms refractory period (max detectable: 150 BPM).
// Increased from 300ms to block the dicrotic notch on fair skin tones,
// which arrives ~300-350ms after the systolic peak and was causing doubled BPM.
const uint32_t REFRACTORY_MS = 400;

uint32_t lastBeatDetectedMs   = 0;
const uint32_t BPM_TIMEOUT_MS = 3000; // Clear BPM if no beat for 3 seconds

// Signal quality gate: envelope must exceed this before beats are accepted.
// Rejects noise, motion artifact, and barely-touching finger.
const float MIN_SIGNAL_QUALITY = 8.0f;

// ========================================================================
// WARMUP VARIABLES
// ========================================================================
// Skips the finger-absent check for WARMUP_MS after placement, giving the
// auto-brightness time to stabilize and preventing false removals on dark skin.
bool     warmupDone  = false;
uint32_t warmupStart = 0;
const uint32_t WARMUP_MS = 500;

// ========================================================================
// SpO2 ESTIMATION VARIABLES
// ========================================================================
float spo2   = 0;
float Rratio = 0;

int32_t irMax  = INT32_MIN, irMin  = INT32_MAX;
int32_t redMax = INT32_MIN, redMin = INT32_MAX;

uint32_t lastSpo2DetectedMs    = 0;
const uint32_t SPO2_TIMEOUT_MS = 10000; // Clear SpO2 if no valid beat for 10 seconds

uint32_t heartBeatTimer = 0;

// ========================================================================
// UTILITY HELPERS
// ========================================================================
// clampf: clamps a float value between lo and hi (inclusive).
// Used to constrain SpO2 estimates to the physiologically valid range (70–100%).
float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// ========================================================================
// HEART SYMBOL HELPER
// ========================================================================
// Draws a filled 13×11 px heart at (x, y) = top-left of its bounding box.
// Built from two filled circles (top lobes) + a filled triangle (bottom point).
//
//   ● ●         ← two circles, r=3, centres at (x+3,y+3) and (x+9,y+3)
//  █████████
//   ███████
//    █████       ← triangle connects the two lobes into a downward point
//     ███
//      █        ← triangle apex at (x+6, y+10)
//
void drawHeart(int16_t x, int16_t y, uint16_t color) {
  display.fillCircle(x + 3,  y + 3, 3, color); // left lobe
  display.fillCircle(x + 9,  y + 3, 3, color); // right lobe
  display.fillTriangle(x, y + 4, x + 12, y + 4, x + 6, y + 10, color);
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
  int yPos        = (SCREEN_HEIGHT - 16) / 2;

  for (int pass = 0; pass < numPasses; pass++) {
    for (int x = SCREEN_WIDTH; x > -textPxWidth; x -= 3) {
      display.clearDisplay();
      display.setTextWrap(false);
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(x, yPos);
      display.print(text);
      display.display();
      if (drainSensor && sensor.getIR() >= 20000) return true;
      delay(8);
    }
  }
  return false;
}

// ========================================================================
// STATE RESET HELPER
// ========================================================================
// Resets all physiological and LED state to safe defaults.
// Called both when a finger is removed and when it is re-placed, ensuring
// that stale BPM/SpO2 values, a wrong DC baseline, and residual LED power
// from a previous placement never carry over into the next measurement.
void resetState() {
  bpm   = 0;  spo2  = 0;
  dcIR  = sensor.getIR();
  dcRED = sensor.getRed();
  env   = 0;
  irMax = INT32_MIN; irMin = INT32_MAX;
  redMax = INT32_MIN; redMin = INT32_MAX;
  lastBeatMs        = millis();
  lastBeatDetectedMs = millis();
  lastSpo2DetectedMs = millis();
  beatCount  = 0;
  warmupDone = false;
  warmupStart = 0;
  irPower = 40; redPower = 40;
  sensor.setPulseAmplitudeIR(irPower);
  sensor.setPulseAmplitudeRed(redPower);
}

// ========================================================================
// SETUP
// ========================================================================
void setup() {
  Serial.begin(115200);

  // --- CROSS-PLATFORM I2C INITIALIZATION ---
#if defined(ESP32)
  Wire.begin(21, 22);
#elif defined(ESP8266)
  Wire.begin(4, 5);
#else
  Wire.begin();
#endif

  // --- OLED INITIALIZATION ---
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed. Check wiring."));
    while (1) delay(10);
  }

  // --- WELCOME SCREEN ---
  // Scrolls the product name once across the display before sensor init.
  // drainSensor=false because sensor.begin() has not been called yet.
  scrollText("Welcome to AikyaNova Pulse Oximeter Demo Board", 1);

  // --- SENSOR INITIALIZATION ---
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
  // sampleRate=400, avg=8 → 50 Hz effective output rate.
  // pulseWidth=411us → 18-bit ADC resolution — critical for dark skin tones.
  sensor.setup(0, 8, 2, 400, 411, 4096);
  sensor.setPulseAmplitudeIR(irPower);
  sensor.setPulseAmplitudeRed(redPower);

  // --- WAIT UNTIL FINGER IS PLACED ---
  Serial.println(F("Waiting for finger..."));
  while (sensor.getIR() < 20000) {
    if (scrollText("Place your finger on sensor", 1, true)) break;
  }

  // Seed baseline after finger is confirmed
  delay(200);
  resetState();
  Serial.println(F("Finger detected. Starting readings."));
}

// ========================================================================
// MAIN LOOP
// ========================================================================
void loop() {
  int32_t irRaw  = sensor.getIR();
  int32_t redRaw = sensor.getRed();

  // ---------------------------------------------------------
  // PHASE 1: Finger Absent Check
  // ---------------------------------------------------------
  if (irRaw < 20000 && warmupDone) {
    resetState();

    while (true) {
      if (scrollText("Place your finger on sensor", 1, true)) break;
      if (sensor.getIR() >= 20000) break;
    }

    delay(200);
    resetState();
    return;
  }

  // Warmup: skip finger-absent check for WARMUP_MS after placement
  if (!warmupDone) {
    if (warmupStart == 0) warmupStart = millis();
    if (millis() - warmupStart >= WARMUP_MS) {
      warmupDone  = true;
      warmupStart = 0;
    }
  }

  // ---------------------------------------------------------
  // PHASE 2: Independent Dual Auto-LED Brightness
  // ---------------------------------------------------------
  if (millis() - lastAdjustTime > 50) {
    bool adjusted = false;

    // IR LED control
    if      (irRaw < 60000  && irPower < 250) { irPower  += 2; adjusted = true; }
    else if (irRaw > 220000 && irPower > 5)   { irPower   = (irPower > 10) ? irPower - 5 : 5; adjusted = true; } // aggressive for very saturated fair skin
    else if (irRaw > 180000 && irPower > 5)   { irPower  -= 2; adjusted = true; }

    // Red LED control (independent — melanin absorbs Red more than IR)
    if      (redRaw < 60000  && redPower < 250) { redPower  += 2; adjusted = true; }
    else if (redRaw > 220000 && redPower > 5)   { redPower   = (redPower > 10) ? redPower - 5 : 5; adjusted = true; }
    else if (redRaw > 180000 && redPower > 5)   { redPower  -= 2; adjusted = true; }

    if (adjusted) {
      sensor.setPulseAmplitudeIR(irPower);
      sensor.setPulseAmplitudeRed(redPower);
    }
    lastAdjustTime = millis();
  }

  // ---------------------------------------------------------
  // PHASE 3: DC Removal & AC Extraction
  // ---------------------------------------------------------
  dcIR  = dcIR  + ((irRaw  - dcIR)  >> SHIFT_DC);
  dcRED = dcRED + ((redRaw - dcRED) >> SHIFT_DC);

  int32_t irAC  = dcIR  - irRaw;
  int32_t redAC = dcRED - redRaw;

  // Track peak-to-peak amplitude for SpO2 calculation
  if (irAC > irMax)   irMax  = irAC;
  if (irAC < irMin)   irMin  = irAC;
  if (redAC > redMax) redMax = redAC;
  if (redAC < redMin) redMin = redAC;

  int32_t x = irAC / 8; // Scale IR signal for Serial Plotter (assumes adcRange=4096)

  // ---------------------------------------------------------
  // PHASE 4: Dynamic Thresholding (Envelope Tracking)
  // ---------------------------------------------------------
  float absx = (x >= 0) ? x : -x;
  env = envAlpha * env + (1.0f - envAlpha) * absx;
  float thresh = 0.5f * env;

  // ---------------------------------------------------------
  // PHASE 5: Peak Detection, BPM & SpO2
  // ---------------------------------------------------------
  // Quality gate: reject beats when signal is too weak or noisy
  bool isPeak = (prev > prev2) && (prev > x) && (prev > thresh) && (env >= MIN_SIGNAL_QUALITY);
  bool beatFired = false;

  if (isPeak) {
    uint32_t now = millis();
    uint32_t dt  = now - lastBeatMs;

    if (dt > REFRACTORY_MS) {
      beatCount++;
      lastBeatMs         = now;
      lastBeatDetectedMs = now;
      beatFired          = true;

      if (beatCount == 1) {
        // First beat after placement: lastBeatMs was set at an arbitrary time,
        // so dt is invalid. Record timestamp only — skip BPM calculation.
      } else {
        // --- BPM ---
        float instBpm = 60000.0f / dt;
        if (bpm == 0) {
          bpm = instBpm;
        } else if (instBpm > bpm * 1.4f) {
          // Plausibility check: >40% sudden jump → likely dicrotic notch or artifact
          bpm = 0.98f * bpm + 0.02f * instBpm;
        } else {
          // Normal smoothing: 30% new beat weight (~3 beats to reflect real HR change)
          bpm = 0.70f * bpm + 0.30f * instBpm;
        }

        // --- SpO2 ---
        float acIRAmp  = (float)(irMax  - irMin);
        float acREDAmp = (float)(redMax - redMin);
        float dcIrF    = (float)dcIR;
        float dcRedF   = (float)dcRED;

        if (acIRAmp > 1 && acREDAmp > 1 && dcIrF > 1 && dcRedF > 1) {
          float nir  = acIRAmp / dcIrF;
          float nred = acREDAmp / dcRedF;
          if (nir > 0.000001f) {
            Rratio = nred / nir;
            float spo2Inst = 110.0f - 25.0f * Rratio;
            spo2Inst = clampf(spo2Inst, 70.0f, 100.0f);
            if (spo2 == 0) spo2 = spo2Inst;
            spo2 = 0.80f * spo2 + 0.20f * spo2Inst; // SpO2 smoothing (20% new beat weight)
            lastSpo2DetectedMs = now;
          }
        }
      }

      // Reset peak-to-peak trackers for the next beat cycle
      irMax = INT32_MIN; irMin = INT32_MAX;
      redMax = INT32_MIN; redMin = INT32_MAX;
    }
  }

  // Stale timeouts: clear readings if no beat detected for the timeout period
  if (bpm  > 0 && (millis() - lastBeatDetectedMs  > BPM_TIMEOUT_MS))  bpm  = 0;
  if (spo2 > 0 && (millis() - lastSpo2DetectedMs  > SPO2_TIMEOUT_MS)) spo2 = 0;

  prev2 = prev;
  prev  = x;

  // ---------------------------------------------------------
  // PHASE 6: Serial Plotter Output
  // ---------------------------------------------------------
  Serial.print("IR_Signal:");   Serial.print(x);
  Serial.print(", Threshold:"); Serial.print(thresh);
  Serial.print(", Beat:");      Serial.print(beatFired ? env : 0);
  Serial.print(", BPM:");       Serial.print(bpm / 2);
  Serial.print(", SpO2:");      Serial.println(spo2 / 2);

  // ---------------------------------------------------------
  // PHASE 7: OLED Display Update
  // ---------------------------------------------------------
  display.clearDisplay();
  display.setTextWrap(false);
  display.setTextColor(SSD1306_WHITE);

  // --- Headers ---
  display.setTextSize(2);
  display.setCursor(0,  5); display.print("BPM");
  display.setCursor(72, 5); display.print("SpO2");

  // --- BPM Value ---
  display.setTextSize(3);
  display.setCursor(0, 30);
  if (bpm > 20 && bpm < 250) {
    display.print((int)bpm);
  } else if (env < MIN_SIGNAL_QUALITY) {
    display.setTextSize(1);
    display.setCursor(0, 30);
    display.print("LOW");
    display.setCursor(0, 42);
    display.print("SIGNAL");
  } else {
    display.print("--");
  }

  // --- SpO2 Value ---
  display.setTextSize(3);
  display.setCursor(72, 30);
  if (spo2 > 70) {
    display.print((int)spo2);
    display.setTextSize(1);
    display.setCursor(117, 30);
    display.print("%");
  } else {
    display.print("--");
  }

  // --- Heart symbol (bottom centre, blinks on each confirmed beat) ---
  if (beatFired) heartBeatTimer = millis() + 150;
  if (millis() < heartBeatTimer) {
    drawHeart(47, 51, SSD1306_WHITE); // 13×11 px heart centred at approximately (53, 57)
  }

  display.display();
}
