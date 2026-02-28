#include <Wire.h>
#include "MAX30105.h"

MAX30105 sensor;

// ---- DC removal ----
int32_t dcIR  = 0;
int32_t dcRED = 0;
const int SHIFT_DC = 4;   // >>4 fast settle

// ---- Peak detect (on IR) ----
int32_t prev = 0, prev2 = 0;
uint32_t lastBeatMs = 0;
float bpm = 0;

// Adaptive threshold
float env = 0;
const float envAlpha = 0.95;
const uint32_t REFRACTORY_MS = 300;

// ---- SpO2 estimation ----
float spo2 = 0;
float Rratio = 0;

// AC amplitude estimation (peak-to-peak between beats)
int32_t irMax = INT32_MIN, irMin = INT32_MAX;
int32_t redMax = INT32_MIN, redMin = INT32_MAX;

// Clamp helper
float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  if (!sensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found");
    while (1);
  }

  sensor.setup(0, 8, 2, 100, 411, 4096);
  sensor.setPulseAmplitudeIR(40);
  sensor.setPulseAmplitudeRed(15);

  delay(200);
  dcIR  = sensor.getIR();
  dcRED = sensor.getRed();
  lastBeatMs = millis();
}

void loop() {
  int32_t irRaw  = sensor.getIR();
  int32_t redRaw = sensor.getRed();

  // DC tracking
  dcIR  = dcIR  + ((irRaw  - dcIR)  >> SHIFT_DC);
  dcRED = dcRED + ((redRaw - dcRED) >> SHIFT_DC);

  // AC components (upright pulse)
  int32_t irAC  = (dcIR  - irRaw);
  int32_t redAC = (dcRED - redRaw);

  // scale for plotting
  int32_t x = irAC / 8;

  // Update envelope for adaptive threshold (use IR)
  float absx = (x >= 0) ? x : -x;
  env = envAlpha * env + (1.0f - envAlpha) * absx;
  float thresh = 0.5f * env;

  // Track min/max between beats for AC amplitude (peak-to-peak)
  if (irAC > irMax) irMax = irAC;
  if (irAC < irMin) irMin = irAC;
  if (redAC > redMax) redMax = redAC;
  if (redAC < redMin) redMin = redAC;

  // Peak detect on scaled IR waveform
  bool isPeak = (prev > prev2) && (prev > x) && (prev > thresh);

  if (isPeak) {
    uint32_t now = millis();
    uint32_t dt = now - lastBeatMs;

    if (dt > REFRACTORY_MS) {
      // BPM
      float instBpm = 60000.0f / dt;
      if (bpm == 0) bpm = instBpm;
      bpm = 0.85f * bpm + 0.15f * instBpm;

      // SpO2 using ratio-of-ratios
      // AC amplitude = peak-to-peak between beats
      float acIR  = (float)(irMax  - irMin);
      float acRED = (float)(redMax - redMin);

      // Prevent divide-by-zero
      float dcIrF  = (float)dcIR;
      float dcRedF = (float)dcRED;

      if (acIR > 1 && acRED > 1 && dcIrF > 1 && dcRedF > 1) {
        float nir  = acIR / dcIrF;
        float nred = acRED / dcRedF;

        if (nir > 0.000001f) {
          Rratio = nred / nir;

          // Educational calibration (common simple approx)
          float spo2Inst = 110.0f - 25.0f * Rratio;

          // Clamp to reasonable range
          spo2Inst = clampf(spo2Inst, 70.0f, 100.0f);

          if (spo2 == 0) spo2 = spo2Inst;
          spo2 = 0.85f * spo2 + 0.15f * spo2Inst;
        }
      }

      // Reset beat window trackers
      irMax = INT32_MIN; irMin = INT32_MAX;
      redMax = INT32_MIN; redMin = INT32_MAX;

      lastBeatMs = now;
    }
  }

  prev2 = prev;
  prev = x;

  // Output columns for Serial Plotter:
  // 1) IR waveform
  // 2) threshold
  // 3) BPM*10
  // 4) SpO2*10
  // 5) R*1000
  Serial.print(x);
  Serial.print(",");
  Serial.print((int)thresh);
  Serial.print(",");
  Serial.print((int)(bpm * 10));
  Serial.print(",");
  Serial.print((int)(spo2 * 10));
  Serial.print(",");
  Serial.println((int)(Rratio * 1000));

  delay(10);
}

