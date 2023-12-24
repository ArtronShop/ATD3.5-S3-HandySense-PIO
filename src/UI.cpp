#include <Arduino.h>
#include <lvgl.h>
#include <ATD3.5-S3.h>
#include "gui/ui.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "UI.h"
#include "HandySense.h"
#include <PinConfigs.h>

static const char * TAG = "UI";

static void o_switch_click_handle(lv_event_t * e) {
  lv_obj_t * target = lv_event_get_target(e);
  int ch = (int) lv_event_get_user_data(e);
  bool value = lv_obj_has_state(target, LV_STATE_CHECKED);

  extern void ControlRelay_Bymanual(String topic, String message, unsigned int length);
  ControlRelay_Bymanual(String("@private/led") + String(ch - 1), value ? "on" : "off", 0);
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

  extern bool pause_wifi_task;
  pause_wifi_task = true;

  xTaskHandle wifi_scan_task_handle;
  static int n = 0;
  scan_finch = false;
  xTaskCreate(wifi_scan_task, "WiFiScanTask", 2 * 1024, &n, 10, &wifi_scan_task_handle);
  wait_wifi_scan = true;
}

static void wifi_save_click_handle(lv_event_t * e) {
  char ssid[64];
  lv_dropdown_get_selected_str(ui_wifi_name, ssid, sizeof(ssid));
  const char * password = lv_textarea_get_text(ui_wifi_password);

  extern void wifiConfig(String ssid, String password)  ;
  wifiConfig(ssid, password);
}

static lv_obj_t * input_target = NULL;
enum {
  INPUT_NUMBER,
  INPUT_TIME
} input_type;

static void number_input(lv_event_t * e) {
  input_target = lv_event_get_target(e);
  input_type = INPUT_NUMBER;
  // lv_label_set_text(ui_number_split_label, ".");
  lv_roller_set_options(ui_number_digit1_input, "0\n1\n2\n3\n4\n5\n6\n7\n8\n9", LV_ROLLER_MODE_NORMAL);
  // lv_roller_set_options(ui_number_digit3_input, "0\n1\n2\n3\n4\n5\n6\n7\n8\n9", LV_ROLLER_MODE_NORMAL);
  lv_obj_add_flag(ui_number_split_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui_number_digit3_input, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui_number_digit4_input, LV_OBJ_FLAG_HIDDEN);

  const char * old_value = lv_label_get_text(input_target);
  int d12 = 0, d3 = 0;
  // sscanf(old_value, "%d.%1d", &d12, &d3);
  d12 = atoi(old_value);
  lv_roller_set_selected(ui_number_digit1_input, (d12 / 10) % 10, LV_ANIM_OFF);
  lv_roller_set_selected(ui_number_digit2_input, d12 % 10, LV_ANIM_OFF);
  // lv_roller_set_selected(ui_number_digit3_input, d3 % 10, LV_ANIM_OFF);

  lv_obj_clear_flag(ui_number_and_time_dialog, LV_OBJ_FLAG_HIDDEN);
}

static void time_input(lv_event_t * e) {
  input_target = lv_event_get_target(e);
  input_type = INPUT_TIME;
  lv_roller_set_options(ui_number_digit1_input, "0\n1\n2", LV_ROLLER_MODE_NORMAL);
  lv_label_set_text(ui_number_split_label, ":");
  lv_obj_clear_flag(ui_number_split_label, LV_OBJ_FLAG_HIDDEN);
  lv_roller_set_options(ui_number_digit3_input, "0\n1\n2\n3\n4\n5", LV_ROLLER_MODE_NORMAL);
  lv_obj_clear_flag(ui_number_digit3_input, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(ui_number_digit4_input, LV_OBJ_FLAG_HIDDEN);

  const char * old_value = lv_label_get_text(input_target);
  int hour = 0, min = 0;
  sscanf(old_value, "%d:%d", &hour, &min);
  lv_roller_set_selected(ui_number_digit1_input, (hour / 10) % 10, LV_ANIM_OFF);
  lv_roller_set_selected(ui_number_digit2_input, hour % 10, LV_ANIM_OFF);
  lv_roller_set_selected(ui_number_digit3_input, (min / 10) % 10, LV_ANIM_OFF);
  lv_roller_set_selected(ui_number_digit4_input, min % 10, LV_ANIM_OFF);

  lv_obj_clear_flag(ui_number_and_time_dialog, LV_OBJ_FLAG_HIDDEN);
}

static void number_time_input_save_click_handle(lv_event_t * e) {
  uint16_t digit1 = lv_roller_get_selected(ui_number_digit1_input);
  uint16_t digit2 = lv_roller_get_selected(ui_number_digit2_input);
  uint16_t digit3 = lv_roller_get_selected(ui_number_digit3_input);
  uint16_t digit4 = lv_roller_get_selected(ui_number_digit4_input);

  lv_obj_add_flag(ui_number_and_time_dialog, LV_OBJ_FLAG_HIDDEN);
  if (input_type == INPUT_NUMBER) {
    lv_label_set_text_fmt(input_target, "%d", (digit1 * 10) + digit2);
  } else if (input_type == INPUT_TIME) {
    lv_label_set_text_fmt(input_target, "%d%d:%d%d", digit1, digit2, digit3, digit4);
  }
  lv_event_send(input_target, LV_EVENT_VALUE_CHANGED, NULL);
}

static void update_temp_soil_ui() ;
static void update_timer_ui() ;

static int get_switch_select_id() {
  lv_obj_t * ui_sw_x[] = {
    ui_switch1_select,
    ui_switch2_select,
    ui_switch3_select,
    ui_switch4_select,
  };
  for (uint8_t i=0;i<4;i++) {
    if (lv_obj_has_state(ui_sw_x[i], LV_STATE_CHECKED)) {
      return i;
    }
  }

  return 0;
}

static void update_temp_soil_ui() {
  extern float Min_Temp[], Max_Temp[], Min_Soil[], Max_Soil[];

  int i = get_switch_select_id();
  lv_label_set_text_fmt(ui_temp_min_input, "%.0f", Min_Temp[i]);
  lv_label_set_text_fmt(ui_temp_max_input, "%.0f", Max_Temp[i]);
  lv_label_set_text_fmt(ui_soil_min_input, "%.0f", Min_Soil[i]);
  lv_label_set_text_fmt(ui_soil_max_input, "%.0f", Max_Soil[i]);
}

static void switch_x_select_click_handle(lv_event_t * e) {
  // lv_event_send(ui_auto_select_btn, LV_EVENT_CLICKED, NULL);
  update_temp_soil_ui();
  update_timer_ui();
}

static void temp_min_value_changed_handle(lv_event_t * e) {
  int i = get_switch_select_id();
  int value = atoi(lv_label_get_text(ui_temp_min_input));
  HandySense_setTempMin(i, value);
}

static void temp_max_value_changed_handle(lv_event_t * e) {
  int i = get_switch_select_id();
  int value = atoi(lv_label_get_text(ui_temp_max_input));
  HandySense_setTempMax(i, value);
}

static void soil_min_value_changed_handle(lv_event_t * e) {
  int i = get_switch_select_id();
  int value = atoi(lv_label_get_text(ui_soil_min_input));
  HandySense_setSoilMin(i, value);
}

static void soil_max_value_changed_handle(lv_event_t * e) {
  int i = get_switch_select_id();
  int value = atoi(lv_label_get_text(ui_soil_max_input));
  HandySense_setSoilMax(i, value);
}

static int get_timer_select_id() {
  lv_obj_t * ui_timer_x[] = {
    ui_timer1_select,
    ui_timer2_select,
    ui_timer3_select,
  };
  for (uint8_t i=0;i<3;i++) {
    if (lv_obj_has_state(ui_timer_x[i], LV_STATE_CHECKED)) {
      return i;
    }
  }

  return 0;
}

static void update_timer_ui() {
  extern unsigned int time_open[4][7][3], time_close[4][7][3];

  int sw_i = get_switch_select_id();
  int timer_i = get_timer_select_id();
  /*
  unsigned int on_time = time_open[sw_i][0][timer_i];
  unsigned int off_time = time_close[sw_i][0][timer_i];
  if ((on_time < ((23 * 60) + 59)) && off_time < ((23 * 60) + 59)) {
    lv_obj_add_state(ui_timer_enable, LV_STATE_CHECKED);
    lv_label_set_text_fmt(ui_time_on_input, "%02d:%02d", on_time / 60, on_time % 60);
    lv_label_set_text_fmt(ui_time_off_input, "%02d:%02d", off_time / 60, off_time % 60);
  } else {
    lv_obj_clear_state(ui_timer_enable, LV_STATE_CHECKED);
    lv_label_set_text(ui_time_on_input, "00:00");
    lv_label_set_text(ui_time_off_input, "00:00");
  }
  */

  lv_obj_t * day_x_enable[] = {
    ui_day1_enable,
    ui_day2_enable,
    ui_day3_enable,
    ui_day4_enable,
    ui_day5_enable,
    ui_day6_enable,
    ui_day7_enable
  };
  bool update_time_on_off_input = false;
  for (uint8_t day_of_week=0;day_of_week<7;day_of_week++) {
    unsigned int on_time = time_open[sw_i][day_of_week][timer_i];
    unsigned int off_time = time_close[sw_i][day_of_week][timer_i];
    if ((on_time < ((23 * 60) + 59)) && off_time < ((23 * 60) + 59)) {
      lv_obj_add_state(day_x_enable[day_of_week], LV_STATE_CHECKED);

      lv_obj_add_state(ui_timer_enable, LV_STATE_CHECKED);
      lv_label_set_text_fmt(ui_time_on_input, "%02d:%02d", on_time / 60, on_time % 60);
      lv_label_set_text_fmt(ui_time_off_input, "%02d:%02d", off_time / 60, off_time % 60);
      update_time_on_off_input = true;
    } else {
      lv_obj_clear_state(day_x_enable[day_of_week], LV_STATE_CHECKED);
    }
  }
  if (!update_time_on_off_input) {
    lv_obj_clear_state(ui_timer_enable, LV_STATE_CHECKED);
    lv_label_set_text(ui_time_on_input, "00:00");
    lv_label_set_text(ui_time_off_input, "00:00");
  }
}

static void timer_x_select_click_handle(lv_event_t * e) {
  update_timer_ui();
}

static void timer_enable_click_handle(lv_event_t * e) {
  int sw_i = get_switch_select_id();
  int timer_i = get_timer_select_id();
  HandySense_updateDisableTimer(sw_i, timer_i);
}

static void time_on_off_value_changed_handle(lv_event_t * e) {
  lv_obj_t * target = lv_event_get_target(e);
  const char * value = lv_label_get_text(target);
  int hour = 0, min = 0;
  sscanf(value, "%d:%d", &hour, &min);
  
  int sw_i = get_switch_select_id();
  int timer_i = get_timer_select_id();
  bool isTimeOn = target == ui_time_on_input;
  HandySense_updateTimeInTimer(sw_i, timer_i, isTimeOn, (hour * 60) + min);
}

static void day_x_click_handle(lv_event_t * e) {
  extern unsigned int time_open[4][7][3], time_close[4][7][3];
  
  lv_obj_t * target = lv_event_get_target(e);

  int sw_i = get_switch_select_id();
  int day_i = (int) lv_event_get_user_data(e);
  int timer_i = get_timer_select_id();
  bool enable = lv_obj_has_state(target, LV_STATE_CHECKED);

  if (enable) {
    {
      const char * value = lv_label_get_text(ui_time_on_input);
      int hour = 0, min = 0;
      sscanf(value, "%d:%d", &hour, &min);
      time_open[sw_i][day_i][timer_i] = (hour * 60) + min;
    }

    {
      const char * value = lv_label_get_text(ui_time_off_input);
      int hour = 0, min = 0;
      sscanf(value, "%d:%d", &hour, &min);
      time_close[sw_i][day_i][timer_i] = (hour * 60) + min;
    }
  } else {
    time_open[sw_i][day_i][timer_i] = 3000;
    time_close[sw_i][day_i][timer_i] = 3000;
  }

  HandySense_updateDayEnableInTimer(sw_i, timer_i, day_i, enable);
}

void UI_init() {
  Display.begin(1); // rotation number 1
  Touch.begin();
  Sound.begin();
  
  // Map peripheral to LVGL
  Display.useLVGL(); // Map display to LVGL
  Touch.useLVGL(); // Map touch screen to LVGL
  Sound.useLVGL(); // Map speaker to LVGL
  
  // Add load your UI function
  ui_init();

  // Add event handle
  lv_obj_add_event_cb(ui_save_btn, number_time_input_save_click_handle, LV_EVENT_CLICKED, NULL);

  // -- Dashboard
  lv_obj_add_event_cb(ui_o1_switch, o_switch_click_handle, LV_EVENT_VALUE_CHANGED, (void*) 1);
  lv_obj_add_event_cb(ui_o2_switch, o_switch_click_handle, LV_EVENT_VALUE_CHANGED, (void*) 2);
  lv_obj_add_event_cb(ui_o3_switch, o_switch_click_handle, LV_EVENT_VALUE_CHANGED, (void*) 3);
  lv_obj_add_event_cb(ui_o4_switch, o_switch_click_handle, LV_EVENT_VALUE_CHANGED, (void*) 4);

  // -- Configs
  lv_obj_add_event_cb(ui_switch1_select, switch_x_select_click_handle, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(ui_switch2_select, switch_x_select_click_handle, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(ui_switch3_select, switch_x_select_click_handle, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(ui_switch4_select, switch_x_select_click_handle, LV_EVENT_CLICKED, NULL);

  lv_obj_add_event_cb(ui_temp_min_input, number_input, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(ui_temp_max_input, number_input, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(ui_soil_min_input, number_input, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(ui_soil_max_input, number_input, LV_EVENT_CLICKED, NULL);
  update_temp_soil_ui();
  lv_obj_add_event_cb(ui_temp_min_input, temp_min_value_changed_handle, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(ui_temp_max_input, temp_max_value_changed_handle, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(ui_soil_min_input, soil_min_value_changed_handle, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(ui_soil_max_input, soil_max_value_changed_handle, LV_EVENT_VALUE_CHANGED, NULL);

  lv_obj_add_event_cb(ui_timer1_select, timer_x_select_click_handle, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(ui_timer2_select, timer_x_select_click_handle, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(ui_timer3_select, timer_x_select_click_handle, LV_EVENT_CLICKED, NULL);

  lv_obj_add_event_cb(ui_timer_enable, timer_enable_click_handle, LV_EVENT_CLICKED, NULL);

  lv_obj_add_event_cb(ui_time_on_input, time_input, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(ui_time_off_input, time_input, LV_EVENT_CLICKED, NULL);
  update_timer_ui();
  lv_obj_add_event_cb(ui_time_on_input, time_on_off_value_changed_handle, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(ui_time_off_input, time_on_off_value_changed_handle, LV_EVENT_VALUE_CHANGED, NULL);

  lv_obj_add_event_cb(ui_day1_enable, day_x_click_handle, LV_EVENT_CLICKED, (void *) 0);
  lv_obj_add_event_cb(ui_day2_enable, day_x_click_handle, LV_EVENT_CLICKED, (void *) 1);
  lv_obj_add_event_cb(ui_day3_enable, day_x_click_handle, LV_EVENT_CLICKED, (void *) 2);
  lv_obj_add_event_cb(ui_day4_enable, day_x_click_handle, LV_EVENT_CLICKED, (void *) 3);
  lv_obj_add_event_cb(ui_day5_enable, day_x_click_handle, LV_EVENT_CLICKED, (void *) 4);
  lv_obj_add_event_cb(ui_day6_enable, day_x_click_handle, LV_EVENT_CLICKED, (void *) 5);
  lv_obj_add_event_cb(ui_day7_enable, day_x_click_handle, LV_EVENT_CLICKED, (void *) 6);

  // -- WiFi
  {
    extern String ssid, password;
    lv_dropdown_set_text(ui_wifi_name, ssid.c_str());
    lv_textarea_set_text(ui_wifi_password, password.c_str());
  }
  lv_obj_add_event_cb(ui_wifi_refresh, wifi_refresh_click_handle, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(ui_wifi_save, wifi_save_click_handle, LV_EVENT_CLICKED, NULL);

  
  lv_disp_load_scr(ui_loading_page);
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

  { // Update sensor value
    extern float temp, humidity, lux_44009, soil;
    lv_label_set_text_fmt(ui_temp_sensor_value, "%.01f °C", temp);
    lv_label_set_text_fmt(ui_humi_sensor_value, "%.01f %%RH", humidity);
    lv_label_set_text_fmt(ui_soil_sensor_value, "%.0f %%", soil);
    lv_label_set_text_fmt(ui_light_sensor_value, "%.01f kLux", lux_44009);
  }

  // WiFi Scan
  if (wait_wifi_scan) {
    if (scan_finch) {
      // Update wifi name dropdown
      lv_dropdown_set_text(ui_wifi_name, NULL);
      lv_dropdown_set_options(ui_wifi_name, "");
      for (int i=0;i<scan_found;i++) {
        lv_dropdown_add_option(ui_wifi_name, WiFi.SSID(i).c_str(), LV_DROPDOWN_POS_LAST);
      }
      lv_obj_add_flag(ui_loading, LV_OBJ_FLAG_HIDDEN);

      WiFi.scanDelete();

      extern bool pause_wifi_task;
      pause_wifi_task = false;

      wait_wifi_scan = false;
    }
  }

  {
    static bool first = true;
    if (first) {
      if (millis() > 3000) {
        lv_disp_load_scr(ui_Index);
        first = false;
      }
    }
  }
}

void UI_updateOutputStatus(int i, bool isOn) {
  lv_obj_t * ox_switch_list[] = {
    ui_o1_switch, 
    ui_o2_switch,
    ui_o3_switch,
    ui_o4_switch
  };

  if (isOn) {
    lv_obj_add_state(ox_switch_list[i], LV_STATE_CHECKED);
  } else {
    lv_obj_clear_state(ox_switch_list[i], LV_STATE_CHECKED);
  }
}

void UI_updateTempSoilMaxMin() {
  update_temp_soil_ui();
}

void UI_updateTimer() {
  update_timer_ui();
}
