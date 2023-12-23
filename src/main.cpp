#include <Arduino.h>
#include "HandySense.h"
#include "UI.h"

#define LED_Y_PIN (5)

void setup() {
  Serial.begin(115200);
  
  UI_init(); // Init LVGL and UI
  HandySense_init(); // Init HandySense, MQTT, WiFi Manager
}

void loop() {
  UI_loop();
  HandySense_loop();
  delay(1);
}
