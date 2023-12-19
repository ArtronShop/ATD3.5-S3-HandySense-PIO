#include <Arduino.h>
#include "UI.h"

#define LED_Y_PIN (5)

void setup() {
  Serial.begin(115200);
  
  // Init LVGL
  UI_init();
}

void loop() {
  UI_loop();
  delay(5);
}
