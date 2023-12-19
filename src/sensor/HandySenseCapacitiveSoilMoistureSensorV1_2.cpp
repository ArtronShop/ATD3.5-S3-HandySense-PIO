#include "HandySenseCapacitiveSoilMoistureSensorV1_2.h"
#include "PinConfigs.h"

int handySenseCapacitiveSoilMoistureSensorV1SoilRead(float* value) {
    float realVolt = analogReadMilliVolts(A1_PIN) / 3.3f * 9.9f;
    Serial.printf("Volt = %.02f\n", realVolt);

    *value = ((realVolt - 1.5f) / (2.8f - 1.5f)) * 100.0f;
    *value = 100.0f - (*value);
    *value = min(max(*value, 0.0f), 100.0f);

    return *value < 0 ? -20 : 0;
}
