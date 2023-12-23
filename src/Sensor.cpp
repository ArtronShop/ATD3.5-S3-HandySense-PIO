#include <Arduino.h>
#include "Sensor.h"

// include your sensor library


void Sensor_init() { // Setup sensor (like void setup())

} 

bool Sensor_getTemp(float*) { // Get Temperature from sensor in °C unit
  return true;
} 

bool Sensor_getHumi(float*) { // Get Humidity from sensor in %RH unit
  return true;
} 


bool Sensor_getSoil(float*) { // Get Soil Humidity from sensor in % unit
  return true;
} 

bool Sensor_getLight(float*) { // Get Light from sensor in lx uint
  return true;
}