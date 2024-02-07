#pragma once
#include "SensorSupport.h"

// Configs use Sensor
#define TEMP_HUMID_SENSOR SHT45 // <<--- Select your temp & humid sensor model
#define SOIL_SENSOR       ANALOG_SOIL_SENSOR // <<--- Select your soil sensor model
#define LIGHT_SENSOR      BH1750 // <<--- Select your light (lux) sensor model


#if TEMP_HUMID_SENSOR == SHT30
#define SHT30_ADDR (0x44) // ADDR pin jump to GND => 0x44, ADDR pin jump to VCC => 0x45
#elif TEMP_HUMID_SENSOR == SHT45
#define SHT45_ADDR (0x44) // SHT45-AD1B => 0x44
#endif

#if SOIL_SENSOR == ANALOG_SOIL_SENSOR
#define SOIL_ANALOG_MIN (2900)
#define SOIL_ANALOG_MAX (1500)
#endif

#if LIGHT_SENSOR == BH1750
#define BH1750_ADDR (0x23) // ADDR pin jump to GND => 0x23, ADDR pin jump to VCC => 0x5C
#endif
