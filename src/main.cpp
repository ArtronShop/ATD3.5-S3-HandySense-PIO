#include <Arduino.h>
#include "HandySense.h"
#include "UI.h"

#define LED_Y_PIN (5)

void setup() {
  Serial.begin(115200);
  
  HandySense_init(); // Init HandySense, MQTT, WiFi Manager
  UI_init(); // Init LVGL and UI
}

void loop() {
  HandySense_loop();
  UI_loop();
  delay(5);
}
