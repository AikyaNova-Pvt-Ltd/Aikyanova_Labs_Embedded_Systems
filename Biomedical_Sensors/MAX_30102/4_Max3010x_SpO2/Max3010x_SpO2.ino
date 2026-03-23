/*
 * ------------------------------------------------------------------------
 * AikyaNova Labs Embedded Systems
 * ------------------------------------------------------------------------
 * Developed by: AikyaNova Labs
 * Firmware: 
 * Reads raw Infrared (IR) and Red optical data from the MAX30105/102 sensor. 
 * - Calculates Heart Rate (BPM) using a dynamic threshold peak-detection algorithm.
 * - Estimates Blood Oxygen (SpO2) using the Red/IR "Ratio of Ratios" method.
 * - Features independent auto-scaling for Red and IR LEDs to dynamically 
 *   compensate for varying melanin levels and tissue thickness.
 * - Outputs parameters to an I2C OLED display and waveforms to the Serial Plotter.
 * * Copyright (c) 2025 AikyaNova™
 * Licensed under the AikyaNova Non-Commercial License.
 * * * Dependencies & Credits:
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
#define OLED_RESET -1 // Share the microcontroller's reset pin
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MAX30105 sensor; // Instantiate the optical sensor object

// ========================================================================
// DUAL AUTO-LED BRIGHTNESS (MELANIN COMPENSATION)
// ========================================================================
// Melanin absorbs visible Red light significantly more than Infrared (IR) light. 
// To ensure a high Signal-to-Noise Ratio (SNR) for accurate SpO2 on all skin tones, 
// we must independently adjust the power of both LEDs dynamically.
byte irPower = 40;            // Starting power for IR LED (~8mA)
byte redPower = 40;           // Starting power for Red LED (~8mA)
unsigned long lastAdjustTime = 0; // Timer to prevent adjusting power too rapidly

// ========================================================================
// DC REMOVAL & BASELINE TRACKING
// ========================================================================
// The DC baseline represents the constant light absorption by tissue and venous blood.
int32_t dcIR  = 0;
int32_t dcRED = 0;
// SHIFT_DC acts as a fast Exponential Moving Average (EMA) filter. 
// Bit-shifting right by 4 is mathematically equivalent to dividing by 16.
const int SHIFT_DC = 4;       

// ========================================================================
// PEAK DETECTION & BPM VARIABLES
// ========================================================================
int32_t prev = 0, prev2 = 0;  // Store past AC signals to detect local waveform peaks
uint32_t lastBeatMs = 0;      // Timestamp of the last detected heartbeat
float bpm = 0;                // Filtered Beats Per Minute

float env = 0;                // Envelope tracking: measures the amplitude of the pulse wave
const float envAlpha = 0.95;  // Envelope smoothing factor (95% old data, 5% new)
const uint32_t REFRACTORY_MS = 300; // Minimum allowed time between valid beats (caps max BPM at 200)

// ========================================================================
// SpO2 ESTIMATION VARIABLES
// ========================================================================
float spo2 = 0;               // Filtered Blood Oxygen Saturation percentage
float Rratio = 0;             // The "Ratio of Ratios" used to calculate SpO2

// Peak-to-Peak tracking variables used to find the AC amplitude for a single beat
int32_t irMax = INT32_MIN, irMin = INT32_MAX;
int32_t redMax = INT32_MIN, redMin = INT32_MAX;

uint32_t heartBeatTimer = 0;  // Timer to control how long the OLED heart icon stays visible

// Helper Function: Clamps a value between a specified minimum and maximum bound
float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// ========================================================================
// SETUP ROUTINE
// ========================================================================
void setup() {
  Serial.begin(115200);

  // --- CROSS-PLATFORM I2C INITIALIZATION ---
#if defined(ESP32)
  Wire.begin(21, 22); // Standard I2C pins for ESP32
#elif defined(ESP8266)
  Wire.begin(4, 5);   // Standard I2C pins for ESP8266 NodeMCU
#else
  Wire.begin();       // Default for Arduino AVR boards
#endif

  // --- OLED INITIALIZATION ---
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed. Check wiring."));
    while(1) delay(10); // Halt execution if screen fails
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 25);
  display.print("Initializing...");
  display.display();

  // --- SENSOR INITIALIZATION ---
  // Fast I2C speed (400kHz) is required to continuously poll the sensor without lagging
  if (!sensor.begin(Wire, I2C_SPEED_FAST)) {
    display.clearDisplay();
    display.setCursor(15, 25);
    display.print("Sensor not found!");
    display.display();
    while (1) delay(10);
  }

  // --- SENSOR CONFIGURATION ---
  // setup(powerLevel, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange)
  // pulseWidth=411us allows maximum integration time, granting 18-bit ADC resolution.
  sensor.setup(0, 8, 2, 100, 411, 4096); 
  
  // Set initial LED brightness levels
  sensor.setPulseAmplitudeIR(irPower);
  sensor.setPulseAmplitudeRed(redPower);

  delay(200); // Allow sensor to stabilize
  dcIR  = sensor.getIR();   // Seed the initial IR DC baseline
  dcRED = sensor.getRed();  // Seed the initial Red DC baseline
  lastBeatMs = millis();
}

// ========================================================================
// MAIN LOOP
// ========================================================================
void loop() {
  // Read raw optical data from the sensor
  int32_t irRaw  = sensor.getIR();
  int32_t redRaw = sensor.getRed();

  // ---------------------------------------------------------
  // PHASE 1: "Place Finger" Check
  // ---------------------------------------------------------
  // If the IR reading drops below 20,000, the sensor is likely reading open air.
  if (irRaw < 20000) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 18);
    display.print("   Place");
    display.setCursor(30, 34);
    display.print("Finger");
    display.display();
    
    // Reset physiological math so bad data doesn't skew moving averages
    bpm = 0; spo2 = 0;
    dcIR = irRaw; dcRED = redRaw;
    
    // Reset LEDs to default baseline to prepare for the next finger placement
    if (irPower != 40 || redPower != 40) {
      irPower = 40; redPower = 40;
      sensor.setPulseAmplitudeIR(irPower);
      sensor.setPulseAmplitudeRed(redPower);
    }
    delay(50);
    return; // Skip the rest of the loop
  }

  // ---------------------------------------------------------
  // PHASE 2: Independent Dual Auto-LED Brightness
  // ---------------------------------------------------------
  // Adjusts LED power every 50ms to keep the raw signal in the "sweet spot" (60k-200k).
  // This prevents dark skin from giving a weak signal and fair skin from saturating the ADC.
  if (millis() - lastAdjustTime > 50) {
    bool adjusted = false;

    // Adjust IR LED
    if (irRaw < 60000 && irPower < 250) { irPower += 2; adjusted = true; }
    else if (irRaw > 200000 && irPower > 5) { irPower -= 2; adjusted = true; }

    // Adjust RED LED independently
    if (redRaw < 60000 && redPower < 250) { redPower += 2; adjusted = true; }
    else if (redRaw > 200000 && redPower > 5) { redPower -= 2; adjusted = true; }

    // Apply new settings to hardware if a change occurred
    if (adjusted) {
      sensor.setPulseAmplitudeIR(irPower);
      sensor.setPulseAmplitudeRed(redPower);
    }
    lastAdjustTime = millis();
  }

  // ---------------------------------------------------------
  // PHASE 3: DC Baseline Tracking & AC Extraction
  // ---------------------------------------------------------
  // Slowly pull the DC baseline toward the current raw reading
  dcIR  = dcIR  + ((irRaw  - dcIR)  >> SHIFT_DC);
  dcRED = dcRED + ((redRaw - dcRED) >> SHIFT_DC);

  // Isolate the AC component (the pulsatile wave). 
  // We subtract raw from DC so the graph peaks upward when blood volume increases.
  int32_t irAC  = (dcIR  - irRaw);
  int32_t redAC = (dcRED - redRaw);

  // Track Peak-to-Peak amplitudes (Max and Min) for the current beat cycle
  // This is required later for the SpO2 calculation.
  if (irAC > irMax) irMax = irAC;
  if (irAC < irMin) irMin = irAC;
  if (redAC > redMax) redMax = redAC;
  if (redAC < redMin) redMin = redAC;

  int32_t x = irAC / 8; // Scale down the IR signal to fit neatly on the Serial Plotter

  // ---------------------------------------------------------
  // PHASE 4: Dynamic Thresholding (Envelope Tracking)
  // ---------------------------------------------------------
  // Calculate the absolute amplitude of the wave to dynamically adjust the peak trigger threshold.
  // This ensures reliable detection on both weak and strong pulses.
  float absx = (x >= 0) ? x : -x;
  env = envAlpha * env + (1.0f - envAlpha) * absx;
  float thresh = 0.5f * env; // Set trigger threshold to 50% of the wave's envelope

  // ---------------------------------------------------------
  // PHASE 5: Peak Detection & Math (BPM and SpO2)
  // ---------------------------------------------------------
  // A peak is validated if the wave changed direction AND is above the noise threshold
  bool isPeak = (prev > prev2) && (prev > x) && (prev > thresh);
  bool beatFired = false;

  if (isPeak) {
    uint32_t now = millis();
    uint32_t dt = now - lastBeatMs; // Time elapsed since last heartbeat

    // Filter out "dicrotic notches" (double-bounces in the pulse wave)
    if (dt > REFRACTORY_MS) {
      
      // --- 1. Calculate BPM ---
      float instBpm = 60000.0f / dt;
      if (bpm == 0) bpm = instBpm;
      bpm = 0.85f * bpm + 0.15f * instBpm; // Smooth BPM using EMA

      // --- 2. Calculate SpO2 ---
      // Get the peak-to-peak AC amplitude for the current beat
      float acIRAmp  = (float)(irMax  - irMin);
      float acREDAmp = (float)(redMax - redMin);
      float dcIrF  = (float)dcIR;
      float dcRedF = (float)dcRED;

      // Ensure valid signals to prevent division by zero
      if (acIRAmp > 1 && acREDAmp > 1 && dcIrF > 1 && dcRedF > 1) {
        
        // Normalize the AC amplitudes against their respective DC baselines
        float nir  = acIRAmp / dcIrF;     
        float nred = acREDAmp / dcRedF;   

        if (nir > 0.000001f) {
          Rratio = nred / nir; // The core "Ratio of Ratios"
          
          // Apply a standard empirical calibration formula for SpO2
          float spo2Inst = 110.0f - 25.0f * Rratio;
          spo2Inst = clampf(spo2Inst, 70.0f, 100.0f); // Clamp to physically realistic values
          
          if (spo2 == 0) spo2 = spo2Inst;
          spo2 = 0.85f * spo2 + 0.15f * spo2Inst; // Smooth SpO2 using EMA
        }
      }

      // --- 3. Reset AC amplitude trackers for the next beat ---
      irMax = INT32_MIN; irMin = INT32_MAX;
      redMax = INT32_MIN; redMin = INT32_MAX;

      lastBeatMs = now;
      beatFired = true; // Flag to trigger UI visuals
    }
  }

  // Shift data points back in time for the next loop's peak detection
  prev2 = prev;
  prev = x;

  // ---------------------------------------------------------
  // PHASE 6: Serial Plotter Output
  // ---------------------------------------------------------
  // Prints labeled variables for the Arduino Serial Plotter.
  // Note: BPM and SpO2 are scaled down (divided by 2) so they fit nicely 
  // on the same visual axis as the raw pulse waveform.
  Serial.print("IR_Signal:");   Serial.print(x);
  Serial.print(", Threshold:"); Serial.print(thresh);
  Serial.print(", Beat:");      Serial.print(beatFired ? env : 0);
  Serial.print(", BPM:");       Serial.print(bpm / 2);
  Serial.print(", SpO2:");      Serial.println(spo2 / 2);

  // ---------------------------------------------------------
  // PHASE 7: OLED Display Update
  // ---------------------------------------------------------
  display.clearDisplay(); // Clear the buffer for the next frame

  // --- Draw Headers ---
  display.setTextSize(2);
  display.setCursor(0, 5);
  display.print("BPM");
  display.setCursor(75, 5);
  display.print("SpO2");

  // --- Draw BPM Value ---
  display.setTextSize(3);
  display.setCursor(0, 30);
  if (bpm > 20 && bpm < 250) {
    display.print((int)bpm);
  } else {
    display.print("--"); // Show dashes if BPM is calculating or out of bounds
  }

  // --- Draw SpO2 Value ---
  display.setCursor(75, 30);
  if (spo2 > 70) {
    display.print((int)spo2);
  } else {
    display.print("--");
  }
  
  // Draw % sign for SpO2 (smaller text aligned to the right)
  display.setTextSize(1);
  display.setCursor(115, 30);
  display.print("%");

  // --- Handle Blinking Heart UI ---
  // If a beat just fired, set the timer 150ms into the future
  if (beatFired) heartBeatTimer = millis() + 150; 
  
  // If the timer hasn't expired, draw the heart circle
  // Adjusted X-coordinate (54) to move it slightly to the left, under the BPM header
  if (millis() < heartBeatTimer) {
    display.fillCircle(54, 55, 6, SSD1306_WHITE); // X=54, Y=55, Radius=6
  }

  display.display(); // Push the completed buffer to the screen
}
