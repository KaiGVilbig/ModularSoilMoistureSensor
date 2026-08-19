#include "config.h"

void setup() {
  Serial.begin(9600);  
}

void loop() {
  int val = analogRead(A0);  

  Serial.print(sensorToPercent(val));
  Serial.println("%");
  delay(1000);
}

float sensorToPercent(int val) {
  // Clamp Values
  if (val > SENSOR_DRY) {
    val = SENSOR_DRY;
  } else if (val < SENSOR_WET) {
    val = SENSOR_WET;
  }

  // Convert val to 0-100 scale. 0 = Dry, 100 = Wet
  int OldRange = (SENSOR_DRY - SENSOR_WET);
  int NewRange = 100; 
  float NewValue = 100 - (((val - SENSOR_WET) * NewRange) / OldRange);
  return NewValue;
}