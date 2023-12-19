#include "HandySenseCapacitiveSoilMoistureSensorV1_2.h"
#include "ESP32AnalogRead.h"

int handySenseCapacitiveSoilMoistureSensorV1SoilRead(float* value) {
    ESP32AnalogRead adc;
    adc.attach(A1);
    float realVolt = adc.readVoltage() / 3.3f * 9.9f;
    Serial.printf("Volt = %.02f\n", realVolt);

    *value = ((realVolt - 1.5f) / (2.8f - 1.5f)) * 100.0f;
    *value = 100.0f - (*value);
    *value = min(max(*value, 0.0f), 100.0f);

    return *value < 0 ? -20 : 0;
}
