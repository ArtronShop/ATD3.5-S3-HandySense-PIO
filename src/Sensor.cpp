#include <Arduino.h>
#include "Sensor.h"
#include "PinConfigs.h"
#include "UserConfigs.h"

static const char * TAG = "Sensor";

// include your sensor library
#include <Wire.h>
#include <ModbusMaster.h>

#if TEMP_HUMID_SENSOR == SHT20
#include <SHT2x.h>
#elif TEMP_HUMID_SENSOR == SHT30
#include <ArtronShop_SHT3x.h>
#elif TEMP_HUMID_SENSOR == SHT45
#include <ArtronShop_SHT45.h>
#endif

#if LIGHT_SENSOR == BH1750
#include <ArtronShop_BH1750.h>
#endif

#if TEMP_HUMID_SENSOR == SHT20
SHT2x sht;
#elif TEMP_HUMID_SENSOR == SHT30
ArtronShop_SHT3x sht3x(SHT30_ADDR, &Wire); // ADDR: 0 => 0x44, ADDR: 1 => 0x45
#elif TEMP_HUMID_SENSOR == SHT45
ArtronShop_SHT45 sht45(&Wire, 0x44); // SHT45-AD1B => 0x44
#elif TEMP_HUMID_SENSOR == ATS_TH
ModbusMaster ats_th;
#elif TEMP_HUMID_SENSOR == XY_MD02
ModbusMaster xy_md02;
#endif

#if LIGHT_SENSOR == BH1750
ArtronShop_BH1750 bh1750(BH1750_ADDR); // Non Jump ADDR: 0x23, Jump ADDR: 0x5C
#elif TEMP_HUMID_SENSOR == ATS_LUX
ModbusMaster ats_lux;
#endif

void Sensor_init() { // Setup sensor (like void setup())
  Wire.begin();
  Serial2.begin(9600, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);

#if TEMP_HUMID_SENSOR == SHT20
  if (!sht.begin()) {
    ESP_LOGE(TAG, "SHT2x not found !");
  }
#elif TEMP_HUMID_SENSOR == SHT30
  if (!sht3x.begin()) {
    ESP_LOGE(TAG, "SHT3x not found !");
  }
#elif TEMP_HUMID_SENSOR == SHT45
  if (!sht45.begin()) {
    ESP_LOGE(TAG, "SHT45 not found !");
  }
#elif TEMP_HUMID_SENSOR == ATS_TH
  ats_th.begin(ATS_TH_ID, Serial2);
#elif TEMP_HUMID_SENSOR == XY_MD02
  xy_md02.begin(XY_MD02_ID, Serial2);
#endif

#if LIGHT_SENSOR == BH1750
  if (!bh1750.begin()) {
    ESP_LOGE(TAG, "BH1750 not found !");
  }
#elif LIGHT_SENSOR == ATS_LUX
  ats_lux.begin(ATS_LUX_ID, Serial2);
#endif
} 

bool Sensor_getTemp(float * value) { // Get Temperature from sensor in °C unit
#if TEMP_HUMID_SENSOR == SHT20
  if (!sht.read()) {
    ESP_LOGE(TAG, "SHT2x read fail");
    return false;
  }
  *value = sht.getTemperature();
#elif TEMP_HUMID_SENSOR == SHT30
  if (!sht3x.measure()) {
    ESP_LOGE(TAG, "SHT3x read fail");
    return false;
  }
  *value = sht3x.temperature();
#elif TEMP_HUMID_SENSOR == SHT45
  if (!sht45.measure()) {
    ESP_LOGE(TAG, "SHT45 read fail");
    return false;
  }
  *value = sht45.temperature();
#elif TEMP_HUMID_SENSOR == ATS_TH
  if (ats_th.readInputRegisters(0x0001, 1) == node.ku8MBSuccess) {
    *value = ats_th.getResponseBuffer(0) / 10.0;
  }
#elif TEMP_HUMID_SENSOR == XY_MD02
  if (ats_th.readInputRegisters(0x0001, 1) == node.ku8MBSuccess) {
    *value = xy_md02.getResponseBuffer(0) / 10.0;
  }
#endif

  return true;
} 

bool Sensor_getHumi(float * value) { // Get Humidity from sensor in %RH unit
#if TEMP_HUMID_SENSOR == SHT20
  // sht.read();
  *value = sht.getHumidity();
#elif TEMP_HUMID_SENSOR == SHT30
  /* if (!sht3x.measure()) {
    return false;
  } */
  *value = sht3x.humidity();
#elif TEMP_HUMID_SENSOR == SHT45
  /* if (!sht45.measure()) {
    return false;
  } */
  *value = sht45.humidity();
#elif TEMP_HUMID_SENSOR == ATS_TH
  if (ats_th.readInputRegisters(0x0002, 1) == node.ku8MBSuccess) {
    *value = ats_th.getResponseBuffer(0) / 10.0;
  }
#elif TEMP_HUMID_SENSOR == XY_MD02
  if (ats_th.readInputRegisters(0x0002, 1) == node.ku8MBSuccess) {
    *value = xy_md02.getResponseBuffer(0) / 10.0;
  }
#endif

  return true;
} 

bool Sensor_getSoil(float * value) { // Get Soil moisture from sensor in % unit
#if SOIL_SENSOR == ANALOG_SOIL_SENSOR
  int raw = analogRead(SOIL_PIN);
  ESP_LOGV(TAG, "A1 analog value : %d", raw);
  *value = map(raw, SOIL_ANALOG_MIN, SOIL_ANALOG_MAX, 0, 100);
  *value = constrain(*value, 0, 100);
#endif

  return true;
} 

bool Sensor_getLight(float * value) { // Get Light from sensor in lux uint
#if LIGHT_SENSOR == BH1750
  *value = bh1750.light();
  if (*value < 0) {
    ESP_LOGE(TAG, "BH1750 read fail");
    return false;
  }
#elif TEMP_HUMID_SENSOR == XY_MD02
  if (ats_lux.readInputRegisters(0x0001, 2) == node.ku8MBSuccess) {
    *value = (uint32_t)((lux.getResponseBuffer(1) << 16) | lux.getResponseBuffer(0));
  }
#endif

  return true;
}