#include <Wire.h>
#include "MAX30105.h"

MAX30105 sensor;

int32_t dcIR = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(21,22);

  if (!sensor.begin(Wire, I2C_SPEED_FAST)) while(1);

  sensor.setup(0, 8, 2, 150, 411, 4096);

  sensor.setPulseAmplitudeIR(40);
  sensor.setPulseAmplitudeRed(15);

  delay(200);
  dcIR = sensor.getIR();
}

void loop() {
  int32_t ir = sensor.getIR();

  // ONLY CHANGE: >>6 to >>4 (faster baseline tracking)
  dcIR = dcIR + ((ir - dcIR) >> 4);

  int32_t ac = dcIR - ir;     // upright peaks
  Serial.println(ac / 8);

  delay(10);
}
