#include <Wire.h>
#include "MAX30105.h"

MAX30105 sensor;

// ---- DC removal (fast settle) ----
int32_t dcIR = 0;
const int SHIFT_DC = 4;     // >>4 fast baseline tracking

// ---- Peak detect state ----
int32_t prev = 0;
int32_t prev2 = 0;
uint32_t lastBeatMs = 0;

float bpm = 0;

// Adaptive threshold (tracks pulse amplitude)
float env = 0;             // envelope of |AC|
const float envAlpha = 0.95;   // closer to 1 = slower

// Refractory period: ignore peaks faster than this
const uint32_t REFRACTORY_MS = 300; // 300ms -> max 200 BPM

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
  dcIR = sensor.getIR();
  lastBeatMs = millis();
}

void loop() {
  int32_t ir = sensor.getIR();

  // DC removal
  dcIR = dcIR + ((ir - dcIR) >> SHIFT_DC);
  int32_t ac = dcIR - ir;          // upright peaks
  int32_t x = ac / 8;              // scaled for plotting

  // Update envelope for adaptive threshold
  float absx = (x >= 0) ? x : -x;
  env = envAlpha * env + (1.0 - envAlpha) * absx;

  // Adaptive threshold: fraction of envelope
  float thresh = 0.5f * env;       // tune 0.4–0.7 if needed

  // Peak detection (local maxima)
  bool isPeak = (prev > prev2) && (prev > x) && (prev > thresh);

  if (isPeak) {
    uint32_t now = millis();
    uint32_t dt = now - lastBeatMs;

    if (dt > REFRACTORY_MS) {
      float instBpm = 60000.0f / dt;

      // Smooth BPM for display (EMA)
      if (bpm == 0) bpm = instBpm;
      bpm = 0.85f * bpm + 0.15f * instBpm;

      lastBeatMs = now;
    }
  }

  // Shift samples
  prev2 = prev;
  prev = x;

  // ---- Output for Serial Plotter/Monitor ----
  // Column1: filtered pulse waveform
  // Column2: threshold line (helps teaching)
  // Column3: BPM (scaled so it appears on plot)
  Serial.print(x);
  Serial.print(",");
  Serial.print((int)thresh);
  Serial.print(",");
  Serial.println((int)(bpm * 10));   // x10 so it shows on plot

  delay(10);
}
