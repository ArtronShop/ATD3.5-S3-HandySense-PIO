#include "HandySenseDHT22.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "PinConfigs.h"

// Real Program part
OneWire oneWire(D3_PIN);
DallasTemperature sensors(&oneWire);

static bool init_sensor = false;
#define CHECK_SENSOR    ({ \
                            if (!init_sensor) {\
                                sensors.begin();\
                                init_sensor = true;\
                            }\
                        })

int handySenseDS18B20TempRead(float* value) {
    CHECK_SENSOR;

	sensors.requestTemperatures(); 
    *value = sensors.getTempCByIndex(0);
        
    return *value < -40 ? -10 : 0;
}
