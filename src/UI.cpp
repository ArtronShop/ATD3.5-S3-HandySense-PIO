#include <Arduino.h>
#include <lvgl.h>
#include <ATD3.5-S3.h>
#include "gui/ui.h"
#include <driver/i2s.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "UI.h"

static const char * TAG = "UI";

static void o_switch_click_handle(lv_event_t * e) {
  lv_obj_t * target = lv_event_get_target(e);
  int ch = (int) lv_event_get_user_data(e);
  bool value = lv_obj_has_state(target, LV_STATE_CHECKED);
  // digitalWrite(o_pin[ch - 1], value);
}

static bool wait_wifi_scan = false;
static bool scan_finch = false;
static int scan_found = 0;

void wifi_scan_task(void *) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  scan_found = WiFi.scanNetworks();
  scan_finch = true;
  vTaskDelete(NULL);
}

static void wifi_refresh_click_handle(lv_event_t * e) {
  lv_obj_clear_flag(ui_loading, LV_OBJ_FLAG_HIDDEN);

  xTaskHandle wifi_scan_task_handle;
  static int n = 0;
  scan_finch = false;
  xTaskCreate(wifi_scan_task, "WiFiScanTask", 2 * 1024, &n, 10, &wifi_scan_task_handle);
  wait_wifi_scan = true;
}

static void wifi_save_click_handle(lv_event_t * e) {
}

// Sound
#define I2S_DOUT      47
#define I2S_BCLK      48
#define I2S_LRC       45
i2s_port_t i2s_num = I2S_NUM_0; // i2s port number

uint16_t beep_sound_wave[800]; // 16 kHz sample rate

void beep() {
  size_t out_bytes = 0;
  i2s_write(i2s_num, beep_sound_wave, sizeof(beep_sound_wave), &out_bytes, 100);
}

void my_inp_feedback(lv_indev_drv_t *indev_driver, uint8_t event) {
    if((event == LV_EVENT_CLICKED) || (event == LV_EVENT_KEY)) {
        beep();
    }
}

void UI_init() {
  // Gen sound beep
  for (uint16_t i=0;i<800;i++) {
    beep_sound_wave[i] = (i % 16) > 8 ? 0x3FFF : 0x0000;
    // Serial.println(beep_sound_wave[i]);
  }
  
  // Setup peripherals
  static const i2s_pin_config_t pin_config = {
    .mck_io_num = I2S_PIN_NO_CHANGE,
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  static i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = 16000,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,  // Interrupt level 1, default 0
      .dma_buf_count = 8,
      .dma_buf_len = 64,
      .use_apll = false,
      .tx_desc_auto_clear = true
  };

  i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
  i2s_set_pin(i2s_num, &pin_config);

  Display.begin(1); // rotation number 1
  Touch.begin();
  
  // Map peripheral to LVGL
  Display.useLVGL(); // Map display to LVGL
  Touch.useLVGL(); // Map touch screen to LVGL
  
  // Add load your UI function
  ui_init();

  // Add event handle

  // -- Dashboard
  lv_obj_add_event_cb(ui_o1_switch, o_switch_click_handle, LV_EVENT_VALUE_CHANGED, (void*) 1);
  lv_obj_add_event_cb(ui_o2_switch, o_switch_click_handle, LV_EVENT_VALUE_CHANGED, (void*) 2);
  lv_obj_add_event_cb(ui_o3_switch, o_switch_click_handle, LV_EVENT_VALUE_CHANGED, (void*) 3);
  lv_obj_add_event_cb(ui_o4_switch, o_switch_click_handle, LV_EVENT_VALUE_CHANGED, (void*) 4);

  // -- Configs

  // -- WiFi
  lv_obj_add_event_cb(ui_wifi_refresh, wifi_refresh_click_handle, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(ui_wifi_save, wifi_save_click_handle, LV_EVENT_CLICKED, NULL);
}

void UI_loop() {
  Display.loop();

  // Time
  extern struct tm timeinfo;
  lv_label_set_text_fmt(ui_time_now_label, "%d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  // Update WiFi status
  if (WiFi.isConnected()) {
    lv_obj_clear_flag(ui_wifi_status_icon, LV_OBJ_FLAG_HIDDEN);

    // Update cloud status
    extern PubSubClient client;
    if (client.connected()) {
      lv_obj_clear_flag(ui_cloud_status_icon, LV_OBJ_FLAG_HIDDEN);
    } else {
      static unsigned long timer = 0;
      if ((timer == 0) || (millis() < timer) || ((millis() - timer) >= 300)) {
        timer = millis();

        if (lv_obj_has_flag(ui_cloud_status_icon, LV_OBJ_FLAG_HIDDEN)) {
          lv_obj_clear_flag(ui_cloud_status_icon, LV_OBJ_FLAG_HIDDEN);
        } else {
          lv_obj_add_flag(ui_cloud_status_icon, LV_OBJ_FLAG_HIDDEN);
        }
      }
    }
  } else {
    static unsigned long timer = 0;
    if ((timer == 0) || (millis() < timer) || ((millis() - timer) >= 300)) {
      timer = millis();

      if (lv_obj_has_flag(ui_wifi_status_icon, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_clear_flag(ui_wifi_status_icon, LV_OBJ_FLAG_HIDDEN);
      } else {
        lv_obj_add_flag(ui_wifi_status_icon, LV_OBJ_FLAG_HIDDEN);
      }
    }
    
    // Update cloud status
    lv_obj_add_flag(ui_cloud_status_icon, LV_OBJ_FLAG_HIDDEN);
  }
  
  // WiFi Scan
  if (wait_wifi_scan) {
    if (scan_finch) {
      // Update wifi name dropdown
      lv_dropdown_set_options(ui_wifi_name, "");
      for (int i=0;i<scan_found;i++) {
        lv_dropdown_add_option(ui_wifi_name, WiFi.SSID(i).c_str(), LV_DROPDOWN_POS_LAST);
      }
      lv_obj_add_flag(ui_loading, LV_OBJ_FLAG_HIDDEN);

      WiFi.scanDelete();
      wait_wifi_scan = false;
    }
  }
}
