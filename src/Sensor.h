#pragma once

#include <stdint.h>
#include <stdbool.h>

void Sensor_init() ; // Setup sensor
bool Sensor_getTemp(float*) ; // Get Temperature from sensor in °C unit
bool Sensor_getHumi(float*) ; // Get Humidity from sensor in %RH unit
bool Sensor_getSoil(float*) ; // Get Soil Humidity from sensor in % unit
bool Sensor_getLight(float*) ; // Get Light from sensor in lx uint

