#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <NTPClient.h>
#include <FS.h>
#include <SPIFFS.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <EEPROM.h>
#include "Artron_DS1338.h"
#include "Sensor.h"
#include "UI.h"
#include "PinConfigs.h"

static void timmer_setting(String topic, byte * payload, unsigned int length) ;
static void SoilMaxMin_setting(String topic, String message, unsigned int length) ;
void TaskWifiStatus(void * pvParameters) ;
void TaskWaitSerial(void * WaitSerial) ;
static void sent_dataTimer(String topic, String message) ;
static void ControlRelay_Bytimmer() ;
static void TempMaxMin_setting(String topic, String message, unsigned int length) ;
void ControlRelay_Bymanual(String topic, String message, unsigned int length) ;

// #define DEBUG

#ifndef DEBUG
#define DEBUG_PRINT(x)    //Serial.print(x)
#define DEBUG_PRINTLN(x)  //Serial.println(x)
#else
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#endif

#define CONFIG_FILE "/configs.json"

// ประกาศใช้เวลาบน Internet
const char* ntpServer = "pool.ntp.org";
const char* nistTime = "time.nist.gov";
const long  gmtOffset_sec = 7 * 3600;
const int   daylightOffset_sec = 0;
int hourNow,
    minuteNow,
    secondNow,
    dayNow,
    monthNow,
    yearNow,
    weekdayNow;

struct tm timeinfo;
String _data; // อาจจะไม่ได้ใช้

// ประกาศตัวแปรสื่อสารกับ web App
byte STX = 02;
byte ETX = 03;
uint8_t START_PATTERN[3] = {0, 111, 222};
const size_t capacity = JSON_OBJECT_SIZE(7) + 320;
DynamicJsonDocument jsonDoc(capacity);

String mqtt_server ,
       mqtt_Client ,
       mqtt_password ,
       mqtt_username ,
       password ,
       mqtt_port,
       ssid ;

String client_old;

// ประกาศใช้ rtc
Artron_DS1338 rtc(&Wire);

// ประกาศใช้ WiFiClient
WiFiClient espClient;
PubSubClient client(espClient);

// ประกาศใช้ WiFiUDP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
int curentTimerError = 0;

const unsigned long eventInterval             = 1 * 1000;          // อ่านค่า temp และ soil sensor ทุก ๆ 1 วินาที
const unsigned long eventInterval_brightness  = 6 * 1000;          // อ่านค่า brightness sensor ทุก ๆ 6 วินาที
unsigned long previousTime_Temp_soil          = 0;
unsigned long previousTime_brightness         = 0;

// ประกาศตัวแปรกำหนดการนับเวลาเริ่มต้น
unsigned long previousTime_Update_data        = 0;
const unsigned long eventInterval_publishData = 2 * 60 * 1000;     // เช่น 2*60*1000 ส่งทุก 2 นาที

float difference_soil                         = 20.00,    // ค่าความชื้นดินแตกต่างกัน +-20 % เมื่อไรส่งค่าขึ้น Web app ทันที
      difference_temp                         = 4.00;     // ค่าอุณหภูมิแตกต่างกัน +- 4 C เมื่อไรส่งค่าขึ้น Web app ทันที

// ประกาศตัวแปรสำหรับเก็บค่าเซ็นเซอร์
float   temp              = 0;
float   humidity          = 0;
float   lux_44009         = 0;
float   soil              = 0;

// Array สำหรับทำ Movie Arg. ของค่าเซ็นเซอร์ทุก ๆ ค่า
static float ma_temp[5];
static float ma_hum[5];
static float ma_soil[5];
static float ma_lux[5];

// สำหรับเก็บค่าเวลาจาก Web App
int t[20];
#define state_On_Off_relay        t[0]
#define value_monday_from_Web     t[1]
#define value_Tuesday_from_Web    t[2]
#define value_Wednesday_from_Web  t[3]
#define value_Thursday_from_Web   t[4]
#define value_Friday_from_Web     t[5]
#define value_Saturday_from_Web   t[6]
#define value_Sunday_from_Web     t[7]
#define value_hour_Open           t[8]
#define value_min_Open            t[9]
#define value_hour_Close          t[11]
#define value_min_Close           t[12]

#define OPEN        1
#define CLOSE       0

#define Open_relay(j) { \
  digitalWrite(relay_pin[j], HIGH); \
  UI_updateOutputStatus(j, true); \
}
  
#define Close_relay(j) { \
  digitalWrite(relay_pin[j], LOW); \
  UI_updateOutputStatus(j, false); \
}

#define connect_WifiStatusToBox     -1

/* new PCB Red */
int relay_pin[4] = { O1_PIN, O2_PIN, O3_PIN, O4_PIN};
#define status_sht31_error          -1
#define status_max44009_error       -1
#define status_soil_error           -1

// ตัวแปรเก็บค่าการตั้งเวลาทำงานอัตโนมัติ
unsigned int time_open[4][7][3] = {{{2000, 2000, 2000}, {2000, 2000, 2000}, {2000, 2000, 2000},
    {2000, 2000, 2000}, {2000, 2000, 2000}, {2000, 2000, 2000}, {2000, 2000, 2000}
  },
  { {2000, 2000, 2000}, {2000, 2000, 2000}, {2000, 2000, 2000},
    {2000, 2000, 2000}, {2000, 2000, 2000}, {2000, 2000, 2000}, {2000, 2000, 2000}
  },
  { {2000, 2000, 2000}, {2000, 2000, 2000}, {2000, 2000, 2000},
    {2000, 2000, 2000}, {2000, 2000, 2000}, {2000, 2000, 2000}, {2000, 2000, 2000}
  },
  { {2000, 2000, 2000}, {2000, 2000, 2000}, {2000, 2000, 2000},
    {2000, 2000, 2000}, {2000, 2000, 2000}, {2000, 2000, 2000}, {2000, 2000, 2000}
  }
};
unsigned int time_close[4][7][3] = {{{2000, 2000, 2000}, {2000, 2000, 2000}, {2000, 2000, 2000},
    {2000, 2000, 2000}, {2000, 2000, 2000}, {2000, 2000, 2000}, {2000, 2000, 2000}
  },
  { {2000, 2000, 2000}, {2000, 2000, 2000}, {2000, 2000, 2000},
    {2000, 2000, 2000}, {2000, 2000, 2000}, {2000, 2000, 2000}, {2000, 2000, 2000}
  },
  { {2000, 2000, 2000}, {2000, 2000, 2000}, {2000, 2000, 2000},
    {2000, 2000, 2000}, {2000, 2000, 2000}, {2000, 2000, 2000}, {2000, 2000, 2000}
  },
  { {2000, 2000, 2000}, {2000, 2000, 2000}, {2000, 2000, 2000},
    {2000, 2000, 2000}, {2000, 2000, 2000}, {2000, 2000, 2000}, {2000, 2000, 2000}
  }
};

float Max_Soil[4], Min_Soil[4];
float Max_Temp[4], Min_Temp[4];

unsigned int statusTimer_open[4] = {1, 1, 1, 1};
unsigned int statusTimer_close[4] = {1, 1, 1, 1};
unsigned int status_manual[4];

unsigned int statusSoil[4];
unsigned int statusTemp[4];

// สถานะการทำงานของ Relay ด้ววยค่า Min Max
int relayMaxsoil_status[4];
int relayMinsoil_status[4];
int relayMaxtemp_status[4];
int relayMintemp_status[4];

int RelayStatus[4];
TaskHandle_t WifiStatus, WaitSerial;
unsigned int oldTimer;

// สถานะการรเชื่อมต่อ wifi
#define cannotConnect   0
#define wifiConnected   1
#define serverConnected 2
#define editDeviceWifi  3
int connectWifiStatus = cannotConnect;

int check_sendData_status = 0;
int check_sendData_toWeb  = 0;
int check_sendData_SoilMinMax = 0;
int check_sendData_tempMinMax = 0;

#define INTERVAL_MESSAGE 600000 //10 นาที
#define INTERVAL_MESSAGE2 1500000 //1 ชม
unsigned long time_restart = 0;

/* --------- Callback function get data from web ---------- */
static void callback(String topic, byte* payload, unsigned int length) {
  //DEBUG_PRINT("Message arrived [");
  DEBUG_PRINT("Message arrived [");
  DEBUG_PRINT(topic);
  DEBUG_PRINT("] ");
  String message;
  for (int i = 0; i < length; i++) {
    message = message + (char)payload[i];
  }
  /* ------- topic timer ------- */
  if (topic.substring(0, 14) == "@private/timer") {
    timmer_setting(topic, payload, length);
    sent_dataTimer(topic, message);
  }
  /* ------- topic manual_control relay ------- */
  else if (topic.substring(0, 12) == "@private/led") {
    status_manual[0] = 0;
    status_manual[1] = 0;
    status_manual[2] = 0;
    status_manual[3] = 0;
    ControlRelay_Bymanual(topic, message, length);
  }
  /* ------- topic Soil min max ------- */
  else if (topic.substring(0, 17) == "@private/max_temp" || topic.substring(0, 17) == "@private/min_temp") {
    TempMaxMin_setting(topic, message, length);
  }
  /* ------- topic Temp min max ------- */
  else if (topic.substring(0, 17) == "@private/max_soil" || topic.substring(0, 17) == "@private/min_soil") {
    SoilMaxMin_setting(topic, message, length);
  }
}

/* ----------------------- Sent Timer --------------------------- */
static void sent_dataTimer(String topic, String message) {
  String _numberTimer = topic.substring(topic.length() - 2).c_str();
  String _payload = "{\"data\":{\"value_timer";
  _payload += _numberTimer;
  _payload += "\":\"";
  _payload += message;
  _payload += "\"}}";
  DEBUG_PRINT("incoming : "); DEBUG_PRINTLN((char*)_payload.c_str());
  client.publish("@shadow/data/update", (char*)_payload.c_str());
}

/* --------- UpdateData_To_Server --------- */
// void UpdateData_To_Server() {
//   String DatatoWeb;
//   char msgtoWeb[200];
//   if (check_sendData_toWeb == 1) {
//     DatatoWeb = "{\"data\": {\"temperature\":" + String(temp) +
//                 ",\"humidity\":" + String(humidity) + ",\"lux\":" +
//                 String(lux_44009) + ",\"soil\":" + String(soil)  + "}}";

//     DEBUG_PRINT("DatatoWeb : "); DEBUG_PRINTLN(DatatoWeb);
//     DatatoWeb.toCharArray(msgtoWeb, (DatatoWeb.length() + 1));
//     if (client.publish("@shadow/data/update", msgtoWeb)) {
//       check_sendData_toWeb = 0;
//       DEBUG_PRINTLN(" Send Data Complete ");
//     }
//     digitalWrite(LEDY, HIGH);
//   }
// }

/* --------- UpdateData_To_Server --------- */
static void UpdateData_To_Server() {
  String DatatoWeb;
    char msgtoWeb[200];
    DatatoWeb = "{\"data\": {\"temperature\":" + String(temp) +
                ",\"humidity\":" + String(humidity) + ",\"lux\":" +
                String(lux_44009) + ",\"soil\":" + String(soil)  + "}}";

    DEBUG_PRINT("DatatoWeb : "); DEBUG_PRINTLN(DatatoWeb);
    DatatoWeb.toCharArray(msgtoWeb, (DatatoWeb.length() + 1));
    if (client.publish("@shadow/data/update", msgtoWeb)) {
      DEBUG_PRINTLN(" Send Data Complete ");
    }
}

/* --------- sendStatus_RelaytoWeb --------- */
static void sendStatus_RelaytoWeb() {
  String _payload;
  char msgUpdateRalay[200];
  if (check_sendData_status == 1) {
    _payload = "{\"data\": {\"led0\":\"" + String(RelayStatus[0]) +
               "\",\"led1\":\"" + String(RelayStatus[1]) +
               "\",\"led2\":\"" + String(RelayStatus[2]) +
               "\",\"led3\":\"" + String(RelayStatus[3]) + "\"}}";
    DEBUG_PRINT("_payload : "); DEBUG_PRINTLN(_payload);
    _payload.toCharArray(msgUpdateRalay, (_payload.length() + 1));
    if (client.publish("@shadow/data/update", msgUpdateRalay)) {
      check_sendData_status = 0;
      DEBUG_PRINTLN(" Send Complete Relay ");
    }
  }
}

/* --------- Respone soilMinMax toWeb --------- */
static void send_soilMinMax() {
  String soil_payload;
  char soilMinMax_data[450];
  if (check_sendData_SoilMinMax == 1) {
    soil_payload =  "{\"data\": {\"min_soil0\":" + String(Min_Soil[0]) + ",\"max_soil0\":" + String(Max_Soil[0]) +
                    ",\"min_soil1\":" + String(Min_Soil[1]) + ",\"max_soil1\":" + String(Max_Soil[1]) +
                    ",\"min_soil2\":" + String(Min_Soil[2]) + ",\"max_soil2\":" + String(Max_Soil[2]) +
                    ",\"min_soil3\":" + String(Min_Soil[3]) + ",\"max_soil3\":" + String(Max_Soil[3]) + "}}";
    DEBUG_PRINT("_payload : "); DEBUG_PRINTLN(soil_payload);
    soil_payload.toCharArray(soilMinMax_data, (soil_payload.length() + 1));
    if (client.publish("@shadow/data/update", soilMinMax_data)) {
      check_sendData_SoilMinMax = 0;
      DEBUG_PRINTLN(" Send Complete min max ");
    }
  }
}

/* --------- Respone tempMinMax toWeb --------- */
static void send_tempMinMax() {
  String temp_payload;
  char tempMinMax_data[400];
  if (check_sendData_tempMinMax == 1) {
    temp_payload =  "{\"data\": {\"min_temp0\":" + String(Min_Temp[0]) + ",\"max_temp0\":" + String(Max_Temp[0]) +
                    ",\"min_temp1\":" + String(Min_Temp[1]) + ",\"max_temp1\":" + String(Max_Temp[1]) +
                    ",\"min_temp2\":" + String(Min_Temp[2]) + ",\"max_temp2\":" + String(Max_Temp[2]) +
                    ",\"min_temp3\":" + String(Min_Temp[3]) + ",\"max_temp3\":" + String(Max_Temp[3]) + "}}";
    DEBUG_PRINT("_payload : "); DEBUG_PRINTLN(temp_payload);
    temp_payload.toCharArray(tempMinMax_data, (temp_payload.length() + 1));
    if (client.publish("@shadow/data/update", tempMinMax_data)) {
      check_sendData_tempMinMax = 0;
    }
  }
}

/* ----------------------- Setting Timer --------------------------- */
void timmer_setting(String topic, byte * payload, unsigned int length) {
  int timer, relay;
  char* str;
  unsigned int count = 0;
  char message_time[50];
  timer = topic.substring(topic.length() - 1).toInt();
  relay = topic.substring(topic.length() - 2, topic.length() - 1).toInt();
  DEBUG_PRINTLN();
  DEBUG_PRINT("timeer     : "); DEBUG_PRINTLN(timer);
  DEBUG_PRINT("relay      : "); DEBUG_PRINTLN(relay);
  for (int i = 0; i < length; i++) {
    message_time[i] = (char)payload[i];
  }
  DEBUG_PRINTLN(message_time);
  str = strtok(message_time, " ,,,:");
  while (str != NULL) {
    t[count] = atoi(str);
    count++;
    str = strtok(NULL, " ,,,:");
  }
  if (state_On_Off_relay == 1) {
    for (int k = 0; k < 7; k++) {
      if (t[k + 1] == 1) {
        time_open[relay][k][timer] = (value_hour_Open * 60) + value_min_Open;
        time_close[relay][k][timer] = (value_hour_Close * 60) + value_min_Close;
      }
      else {
        time_open[relay][k][timer] = 3000;
        time_close[relay][k][timer] = 3000;
      }
      int address = ((((relay * 7 * 3) + (k * 3) + timer) * 2) * 2) + 2100;
      EEPROM.write(address, time_open[relay][k][timer] / 256);
      EEPROM.write(address + 1, time_open[relay][k][timer] % 256);
      EEPROM.write(address + 2, time_close[relay][k][timer] / 256);
      EEPROM.write(address + 3, time_close[relay][k][timer] % 256);
      EEPROM.commit();
      DEBUG_PRINT("time_open  : "); DEBUG_PRINTLN(time_open[relay][k][timer]);
      DEBUG_PRINT("time_close : "); DEBUG_PRINTLN(time_close[relay][k][timer]);
    }
  }
  else if (state_On_Off_relay == 0) {
    for (int k = 0; k < 7; k++) {
      time_open[relay][k][timer] = 3000;
      time_close[relay][k][timer] = 3000;
      int address = ((((relay * 7 * 3) + (k * 3) + timer) * 2) * 2) + 2100;
      EEPROM.write(address, time_open[relay][k][timer] / 256);
      EEPROM.write(address + 1, time_open[relay][k][timer] % 256);
      EEPROM.write(address + 2, time_close[relay][k][timer] / 256);
      EEPROM.write(address + 3, time_close[relay][k][timer] % 256);
      EEPROM.commit();
      DEBUG_PRINT("time_open  : "); DEBUG_PRINTLN(time_open[relay][k][timer]);
      DEBUG_PRINT("time_close : "); DEBUG_PRINTLN(time_close[relay][k][timer]);
    }
  }
  else {
    DEBUG_PRINTLN("Not enabled timer, Day !!!");
  }
  UI_updateTimer();
}

void sendUpdateTimerToServer(uint8_t relay, uint8_t timer) {
  uint8_t enable = 0;
  uint8_t day_enable[7];
  uint8_t time_on_hour = 0, time_on_min = 0, time_off_hour = 0, time_off_min = 0;
  for (int k = 0; k < 7; k++) {
    day_enable[k] = ((time_open[relay][k][timer] <= ((23 * 60) + 59)) && (time_close[relay][k][timer] <= ((23 * 60) + 59))) ? 1 : 0;
    if (day_enable[k]) {
      enable = 1;
      time_on_hour = time_open[relay][k][timer] / 60;
      time_on_min = time_open[relay][k][timer] % 60;
      time_off_hour = time_close[relay][k][timer] / 60;
      time_off_min = time_close[relay][k][timer] % 60;
    }
  }

  char payload[64];
  memset(payload, 0, sizeof(payload));
  sprintf(payload, "{\"data\":{\"value_timer%d%d\":\"%d,%d,%d,%d,%d,%d,%d,%d,%02d:%02d:00,%02d:%02d:00\"}}",
    relay, timer, enable, day_enable[0], day_enable[1], day_enable[2], day_enable[3], day_enable[4], day_enable[5], day_enable[6], time_on_hour, time_on_min, time_off_hour, time_off_min
  );
  DEBUG_PRINT("update shadow : "); DEBUG_PRINTLN(payload);
  client.publish("@shadow/data/update", payload);
}

void HandySense_updateTimeInTimer(uint8_t relay, uint8_t timer, bool isTimeOn, uint16_t time) {
  bool found_enable = false;
  for (int k = 0; k < 7; k++) {
    unsigned int on_time = time_open[relay][k][timer];
    unsigned int off_time = time_close[relay][k][timer];
    if ((on_time < ((23 * 60) + 59)) && off_time < ((23 * 60) + 59)) {
      if (isTimeOn) {
        time_open[relay][k][timer] = time;
        if (time_close[relay][k][timer] > ((23 * 60) + 59)) {
            time_close[relay][k][timer] = time + 1;
        }
      } else {
        if (time_open[relay][k][timer] > ((23 * 60) + 59)) {
            time_open[relay][k][timer] = time;
            time_close[relay][k][timer] = time + 1;
        } else {
          time_close[relay][k][timer] = time;
        }
      }

      int address = ((((relay * 7 * 3) + (k * 3) + timer) * 2) * 2) + 2100;
      EEPROM.write(address, time_open[relay][k][timer] / 256);
      EEPROM.write(address + 1, time_open[relay][k][timer] % 256);
      EEPROM.write(address + 2, time_close[relay][k][timer] / 256);
      EEPROM.write(address + 3, time_close[relay][k][timer] % 256);
      EEPROM.commit();
      DEBUG_PRINT("time_open  : "); DEBUG_PRINTLN(time_open[relay][k][timer]);
      DEBUG_PRINT("time_close : "); DEBUG_PRINTLN(time_close[relay][k][timer]);

      found_enable = true;
    }
  }

  if (!found_enable) {
    // Set all timer to same time
    for (int k = 0; k < 7; k++) {
      if (isTimeOn) {
        time_open[relay][k][timer] = time;
        if ((time_close[relay][k][timer] > ((23 * 60) + 59)) || (time_close[relay][k][timer] <= time)) {
            time_close[relay][k][timer] = time + 1;
        }
      } else {
        if ((time_open[relay][k][timer] > ((23 * 60) + 59)) || (time_open[relay][k][timer] >= time)) {
            time_open[relay][k][timer] = time;
            time_close[relay][k][timer] = time + 1;
        } else {
          time_close[relay][k][timer] = time;
        }
      }

      int address = ((((relay * 7 * 3) + (k * 3) + timer) * 2) * 2) + 2100;
      EEPROM.write(address, time_open[relay][k][timer] / 256);
      EEPROM.write(address + 1, time_open[relay][k][timer] % 256);
      EEPROM.write(address + 2, time_close[relay][k][timer] / 256);
      EEPROM.write(address + 3, time_close[relay][k][timer] % 256);
      EEPROM.commit();
      DEBUG_PRINT("time_open  : "); DEBUG_PRINTLN(time_open[relay][k][timer]);
      DEBUG_PRINT("time_close : "); DEBUG_PRINTLN(time_close[relay][k][timer]);
    }
  }

  UI_updateTimer();
  sendUpdateTimerToServer(relay, timer);
}

void HandySense_updateDayEnableInTimer(uint8_t relay, uint8_t timer, uint8_t day, bool enable) {
  int address = ((((relay * 7 * 3) + (day * 3) + timer) * 2) * 2) + 2100;
  EEPROM.write(address, time_open[relay][day][timer] / 256);
  EEPROM.write(address + 1, time_open[relay][day][timer] % 256);
  EEPROM.write(address + 2, time_close[relay][day][timer] / 256);
  EEPROM.write(address + 3, time_close[relay][day][timer] % 256);
  EEPROM.commit();
  DEBUG_PRINT("time_open  : "); DEBUG_PRINTLN(time_open[relay][day][timer]);
  DEBUG_PRINT("time_close : "); DEBUG_PRINTLN(time_close[relay][day][timer]);

  UI_updateTimer();
  sendUpdateTimerToServer(relay, timer);
}

void HandySense_updateDisableTimer(uint8_t relay, uint8_t timer) {
  // Set all timer to same time
  for (int k = 0; k < 7; k++) {
    time_open[relay][k][timer] = 3000;
    time_close[relay][k][timer] = 3000;

    int address = ((((relay * 7 * 3) + (k * 3) + timer) * 2) * 2) + 2100;
    EEPROM.write(address, time_open[relay][k][timer] / 256);
    EEPROM.write(address + 1, time_open[relay][k][timer] % 256);
    EEPROM.write(address + 2, time_close[relay][k][timer] / 256);
    EEPROM.write(address + 3, time_close[relay][k][timer] % 256);
    EEPROM.commit();
    DEBUG_PRINT("time_open  : "); DEBUG_PRINTLN(time_open[relay][k][timer]);
    DEBUG_PRINT("time_close : "); DEBUG_PRINTLN(time_close[relay][k][timer]);
  }

  UI_updateTimer();
  sendUpdateTimerToServer(relay, timer);
}

/* ------------ Control Relay By Timmer ------------- */
static void ControlRelay_Bytimmer() {
  int curentTimer;
  int dayofweek;
  bool getTimeFromInternet = false;
  if (WiFi.status() == WL_CONNECTED) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer, nistTime);
    if (getLocalTime(&timeinfo)) {
      getTimeFromInternet = true;
    }
  }
  if (!getTimeFromInternet) {
    rtc.read(&timeinfo);
    DEBUG_PRINT("USE RTC 1");
  } else {
    rtc.write(&timeinfo); // update time in RTC
  }
  
  yearNow     = timeinfo.tm_year + 1900;
  monthNow    = timeinfo.tm_mon + 1;
  dayNow      = timeinfo.tm_mday;
  weekdayNow  = timeinfo.tm_wday;
  hourNow     = timeinfo.tm_hour;
  minuteNow   = timeinfo.tm_min;
  secondNow   = timeinfo.tm_sec;

  curentTimer = (hourNow * 60) + minuteNow;
  dayofweek = weekdayNow - 1;
  
  //DEBUG_PRINT("curentTimer : "); DEBUG_PRINTLN(curentTimer);
  /* check curentTimer => 0-1440 */
  if (curentTimer < 0 || curentTimer > 1440) {
    curentTimerError = 1;
    DEBUG_PRINT("curentTimerError : "); DEBUG_PRINTLN(curentTimerError);
  } else {
    curentTimerError = 0;
    if (dayofweek == -1) {
      dayofweek = 6;
    }
    //DEBUG_PRINT("dayofweek   : "); DEBUG_PRINTLN(dayofweek);
    if (curentTimer != oldTimer) {
      for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 3; j++) {
          if ((time_open[i][dayofweek][j] == 3000) && (time_close[i][dayofweek][j] == 3000)) { // if timer disable
            //        Close_relay(i);
            //        DEBUG_PRINTLN(" Not check day, Not Working relay");
          } else if ((curentTimer >= time_open[i][dayofweek][j]) && (curentTimer < time_close[i][dayofweek][j])) {
            RelayStatus[i] = 1;
            check_sendData_status = 1;
            Open_relay(i);
            DEBUG_PRINTLN("timer On");
            DEBUG_PRINT("curentTimer : "); DEBUG_PRINTLN(curentTimer);
            DEBUG_PRINT("oldTimer    : "); DEBUG_PRINTLN(oldTimer);
          } else {
            RelayStatus[i] = 0;
            check_sendData_status = 1;
            Close_relay(i);
            DEBUG_PRINTLN("timer Off");
            DEBUG_PRINT("curentTimer : "); DEBUG_PRINTLN(curentTimer);
            DEBUG_PRINT("oldTimer    : "); DEBUG_PRINTLN(oldTimer);
          }
          
        }
      }
      oldTimer = curentTimer;
    }
  }
}

/* ----------------------- Manual Control --------------------------- */
void ControlRelay_Bymanual(String topic, String message, unsigned int length) {
  String manual_message = message;
  int manual_relay = topic.substring(topic.length() - 1).toInt();
  DEBUG_PRINTLN();
  DEBUG_PRINT("manual_message : "); DEBUG_PRINTLN(manual_message);
  DEBUG_PRINT("manual_relay   : "); DEBUG_PRINTLN(manual_relay);
  //if (status_manual[manual_relay] == 0) {
    //status_manual[manual_relay] = 1;
    if (manual_message == "on") {
      Open_relay(manual_relay);
      RelayStatus[manual_relay] = 1;
      DEBUG_PRINTLN("ON man");
    }
    else if (manual_message == "off") {
      Close_relay(manual_relay);
      RelayStatus[manual_relay] = 0;
      DEBUG_PRINTLN("OFF man");
    }
    check_sendData_status = 1;
  //}
}

/* ----------------------- SoilMaxMin_setting --------------------------- */
void SoilMaxMin_setting(String topic, String message, unsigned int length) {
  String soil_message = message;
  String soil_topic = topic;
  int Relay_SoilMaxMin = topic.substring(topic.length() - 1).toInt();
  if (soil_topic.substring(9, 12) == "max") {
    relayMaxsoil_status[Relay_SoilMaxMin] = topic.substring(topic.length() - 1).toInt();
    Max_Soil[Relay_SoilMaxMin] = soil_message.toInt();
    EEPROM.write(Relay_SoilMaxMin + 2000,  Max_Soil[Relay_SoilMaxMin]);
    EEPROM.commit();
    check_sendData_SoilMinMax = 1;
    DEBUG_PRINT("Max_Soil : "); DEBUG_PRINTLN(Max_Soil[Relay_SoilMaxMin]);
  }
  else if (soil_topic.substring(9, 12) == "min") {
    relayMinsoil_status[Relay_SoilMaxMin] = topic.substring(topic.length() - 1).toInt();
    Min_Soil[Relay_SoilMaxMin] = soil_message.toInt();
    EEPROM.write(Relay_SoilMaxMin + 2004,  Min_Soil[Relay_SoilMaxMin]);
    EEPROM.commit();
    check_sendData_SoilMinMax = 1;
    DEBUG_PRINT("Min_Soil : "); DEBUG_PRINTLN(Min_Soil[Relay_SoilMaxMin]);
  }
  UI_updateTempSoilMaxMin();
}

/* ----------------------- TempMaxMin_setting --------------------------- */
void TempMaxMin_setting(String topic, String message, unsigned int length) {
  String temp_message = message;
  String temp_topic = topic;
  int Relay_TempMaxMin = topic.substring(topic.length() - 1).toInt();
  if (temp_topic.substring(9, 12) == "max") {
    Max_Temp[Relay_TempMaxMin] = temp_message.toInt();
    EEPROM.write(Relay_TempMaxMin + 2008, Max_Temp[Relay_TempMaxMin]);
    EEPROM.commit();
    check_sendData_tempMinMax = 1;
    DEBUG_PRINT("Max_Temp : "); DEBUG_PRINTLN(Max_Temp[Relay_TempMaxMin]);
  }
  else if (temp_topic.substring(9, 12) == "min") {
    Min_Temp[Relay_TempMaxMin] = temp_message.toInt();
    EEPROM.write(Relay_TempMaxMin + 2012,  Min_Temp[Relay_TempMaxMin]);
    EEPROM.commit();
    check_sendData_tempMinMax = 1;
    DEBUG_PRINT("Min_Temp : "); DEBUG_PRINTLN(Min_Temp[Relay_TempMaxMin]);
  }
  UI_updateTempSoilMaxMin();
}

void HandySense_setTempMin(uint8_t relay, int value) {
  Min_Temp[relay] = value;
  EEPROM.write(relay + 2012,  Min_Temp[relay]);
  EEPROM.commit();
  DEBUG_PRINT("Min_Temp : "); DEBUG_PRINTLN(Min_Temp[relay]);

  char payload[64];
  memset(payload, 0, sizeof(payload));
  sprintf(payload, "{\"data\":{\"min_temp%d\":%.0f}}",
    relay, Min_Temp[relay]
  );
  DEBUG_PRINT("update shadow : "); DEBUG_PRINTLN(payload);
  client.publish("@shadow/data/update", payload);
}

void HandySense_setTempMax(uint8_t relay, int value) {
  Max_Temp[relay] = value;
  EEPROM.write(relay + 2008, Max_Temp[relay]);
  EEPROM.commit();
  DEBUG_PRINT("Max_Temp : "); DEBUG_PRINTLN(Max_Temp[relay]);
  
  char payload[64];
  memset(payload, 0, sizeof(payload));
  sprintf(payload, "{\"data\":{\"max_temp%d\":%.0f}}",
    relay, Max_Temp[relay]
  );
  DEBUG_PRINT("update shadow : "); DEBUG_PRINTLN(payload);
  client.publish("@shadow/data/update", payload);
}

void HandySense_setSoilMin(uint8_t relay, int value) {
  Min_Soil[relay] = value;
  EEPROM.write(relay + 2004,  Min_Soil[relay]);
  EEPROM.commit();
  DEBUG_PRINT("Min_Soil : "); DEBUG_PRINTLN(Min_Soil[relay]);

  char payload[64];
  memset(payload, 0, sizeof(payload));
  sprintf(payload, "{\"data\":{\"min_soil%d\":%.0f}}",
    relay, Min_Soil[relay]
  );
  DEBUG_PRINT("update shadow : "); DEBUG_PRINTLN(payload);
  client.publish("@shadow/data/update", payload);
}

void HandySense_setSoilMax(uint8_t relay, int value) {
  Max_Soil[relay] = value;
  EEPROM.write(relay + 2000, Max_Soil[relay]);
  EEPROM.commit();
  DEBUG_PRINT("Max_Soil : "); DEBUG_PRINTLN(Max_Soil[relay]);
  
  char payload[64];
  memset(payload, 0, sizeof(payload));
  sprintf(payload, "{\"data\":{\"max_soil%d\":%.0f}}",
    relay, Max_Soil[relay]
  );
  DEBUG_PRINT("update shadow : "); DEBUG_PRINTLN(payload);
  client.publish("@shadow/data/update", payload);
}

/* ----------------------- soilMinMax_ControlRelay --------------------------- */
static void ControlRelay_BysoilMinMax() {
  for (int k = 0; k < 4; k++) {
    if (Min_Soil[k] != 0 && Max_Soil[k] != 0) {
      if (soil < Min_Soil[k]) {
        //DEBUG_PRINT("statusSoilMin"); DEBUG_PRINT(k); DEBUG_PRINT(" : "); DEBUG_PRINTLN(statusTemp[k]);
        if (statusSoil[k] == 0) {
          Open_relay(k);
          statusSoil[k] = 1;
          RelayStatus[k] = 1;
          check_sendData_status = 1;
          //check_sendData_toWeb = 1;
          DEBUG_PRINTLN("soil On");
        }
      }
      else if (soil > Max_Soil[k]) {
        //DEBUG_PRINT("statusSoilMax"); DEBUG_PRINT(k); DEBUG_PRINT(" : "); DEBUG_PRINTLN(statusTemp[k]);
        if (statusSoil[k] == 1) {
          Close_relay(k);
          statusSoil[k] = 0;
          RelayStatus[k] = 0;
          check_sendData_status = 1;
          //check_sendData_toWeb = 1;
          DEBUG_PRINTLN("soil Off");
        }
      }
    }
  }
}

/* ----------------------- tempMinMax_ControlRelay --------------------------- */
void ControlRelay_BytempMinMax() {
  for (int g = 0; g < 4; g++) {
    if (Min_Temp[g] != 0 && Max_Temp[g] != 0) {
      if (temp < Min_Temp[g]) {
        //DEBUG_PRINT("statusTempMin"); DEBUG_PRINT(g); DEBUG_PRINT(" : "); DEBUG_PRINTLN(statusTemp[g]);
        if (statusTemp[g] == 1) {
          Close_relay(g);
          statusTemp[g] = 0;
          RelayStatus[g] = 0;
          check_sendData_status = 1;
          //check_sendData_toWeb = 1;
          DEBUG_PRINTLN("temp Off");
        }
      }
      else if (temp > Max_Temp[g]) {
        //DEBUG_PRINT("statusTempMax"); DEBUG_PRINT(g); DEBUG_PRINT(" : "); DEBUG_PRINTLN(statusTemp[g]);
        if (statusTemp[g] == 0) {
          Open_relay(g);
          statusTemp[g] = 1;
          RelayStatus[g] = 1;
          check_sendData_status = 1;
          //check_sendData_toWeb = 1;
          DEBUG_PRINTLN("temp On");
        }
      }
    }
  }
}

/* ----------------------- Mode for calculator sensor i2c --------------------------- */
void printLocalTime() {
  if (!getLocalTime(&timeinfo)) {
    DEBUG_PRINTLN("Failed to obtain time");
    return;
  } //DEBUG_PRINTLN(&timeinfo, "%A, %d %B %Y %H:%M:%S");
}

/* ----------------------- Set All Config --------------------------- */
void setAll_config() {
  for (int b = 0; b < 4; b++) {
    Max_Soil[b] = EEPROM.read(b + 2000);
    Min_Soil[b] = EEPROM.read(b + 2004);
    Max_Temp[b] = EEPROM.read(b + 2008);
    Min_Temp[b] = EEPROM.read(b + 2012);
    if (Max_Soil[b] >= 255) {
      Max_Soil[b] = 0;
    }
    if (Min_Soil[b] >= 255) {
      Min_Soil[b] = 0;
    }
    if (Max_Temp[b] >= 255) {
      Max_Temp[b] = 0;
    }
    if (Min_Temp[b] >= 255) {
      Min_Temp[b] = 0;
    }
    DEBUG_PRINT("Max_Soil   ");  DEBUG_PRINT(b); DEBUG_PRINT(" : "); DEBUG_PRINTLN(Max_Soil[b]);
    DEBUG_PRINT("Min_Soil   ");  DEBUG_PRINT(b); DEBUG_PRINT(" : "); DEBUG_PRINTLN(Min_Soil[b]);
    DEBUG_PRINT("Max_Temp   ");  DEBUG_PRINT(b); DEBUG_PRINT(" : "); DEBUG_PRINTLN(Max_Temp[b]);
    DEBUG_PRINT("Min_Temp   ");  DEBUG_PRINT(b); DEBUG_PRINT(" : "); DEBUG_PRINTLN(Min_Temp[b]);
  }
  int count_in = 0;
  for (int eeprom_relay = 0; eeprom_relay < 4; eeprom_relay++) {
    for (int eeprom_timer = 0; eeprom_timer < 3; eeprom_timer++) {
      for (int dayinweek = 0; dayinweek < 7; dayinweek++) {
        int eeprom_address = ((((eeprom_relay * 7 * 3) + (dayinweek * 3) + eeprom_timer) * 2) * 2) + 2100;
        time_open[eeprom_relay][dayinweek][eeprom_timer] = (EEPROM.read(eeprom_address) * 256) + (EEPROM.read(eeprom_address + 1));
        time_close[eeprom_relay][dayinweek][eeprom_timer] = (EEPROM.read(eeprom_address + 2) * 256) + (EEPROM.read(eeprom_address + 3));

        if (time_open[eeprom_relay][dayinweek][eeprom_timer] >= 2000) {
          time_open[eeprom_relay][dayinweek][eeprom_timer] = 3000;
        }
        if (time_close[eeprom_relay][dayinweek][eeprom_timer] >= 2000) {
          time_close[eeprom_relay][dayinweek][eeprom_timer] = 3000;
        }
        DEBUG_PRINT("cout       : "); DEBUG_PRINTLN(count_in);
        DEBUG_PRINT("time_open  : "); DEBUG_PRINTLN(time_open[eeprom_relay][dayinweek][eeprom_timer]);
        DEBUG_PRINT("time_close : "); DEBUG_PRINTLN(time_close[eeprom_relay][dayinweek][eeprom_timer]);
        count_in++;
      }
    }
  }
}

/* ----------------------- Delete All Config --------------------------- */
void Delete_All_config() {
  for (int b = 2000; b < 4096; b++) {
    EEPROM.write(b, 255);
    EEPROM.commit();
  }
}

/* ----------------------- Add and Edit device || Edit Wifi --------------------------- */
void Edit_device_wifi() {
  connectWifiStatus = editDeviceWifi;
  Serial.write(START_PATTERN, 3);               // ส่งข้อมูลชนิดไบต์ ส่งตัวอักษรไปบนเว็บ
  Serial.flush();                               // เครียบัฟเฟอร์ให้ว่าง
  File configs_file = SPIFFS.open(CONFIG_FILE);
  deserializeJson(jsonDoc, configs_file);             // คือการรับหรืออ่านข้อมูล jsonDoc
  configs_file.close();
  client_old = jsonDoc["client"].as<String>();  // เก็บค่า client เก่าเพื่อเช็คกับ client ใหม่ ในการ reset Wifi ไม่ให้ลบค่า Config อื่นๆ
  Serial.write(STX);                            // 02 คือเริ่มส่ง
  serializeJsonPretty(jsonDoc, Serial);         // ส่งข่อมูลของ jsonDoc ไปบนเว็บ
  Serial.write(ETX);
  delay(1000);
  Serial.write(START_PATTERN, 3);               // ส่งข้อมูลชนิดไบต์ ส่งตัวอักษรไปบนเว็บ
  Serial.flush();
  jsonDoc["server"]   = NULL;
  jsonDoc["client"]   = NULL;
  jsonDoc["pass"]     = NULL;
  jsonDoc["user"]     = NULL;
  jsonDoc["password"] = NULL;
  jsonDoc["port"]     = NULL;
  jsonDoc["ssid"]     = NULL;
  jsonDoc["command"]  = NULL;
  Serial.write(STX);                      // 02 คือเริ่มส่ง
  serializeJsonPretty(jsonDoc, Serial);   // ส่งข่อมูลของ jsonDoc ไปบนเว็บ
  Serial.write(ETX);                      // 03 คือจบ
}

/* -------- webSerialJSON function ------- */
void webSerialJSON() {
  while (Serial.available() > 0) {
    Serial.setTimeout(10000);
    DeserializationError err = deserializeJson(jsonDoc, Serial);
    if (err == DeserializationError::Ok) {
      String command  =  jsonDoc["command"].as<String>();
      bool isValidData  =  !jsonDoc["client"].isNull();
      if (command == "restart") {
        delay(100);
        ESP.restart();
      }
      if (isValidData) {
        /* ------------------WRITING----------------- */
        File configs_file = SPIFFS.open(CONFIG_FILE, FILE_WRITE);
        serializeJson(jsonDoc, configs_file);
        configs_file.close();
        // ถ้าไม่เหมือนคือเพิ่มอุปกรณ์ใหม่ // ถ้าเหมือนคือการเปลี่ยน wifi
        if (client_old != jsonDoc["client"].as<String>()) {
          Delete_All_config();
        }
        delay(100);
        ESP.restart();
      }
    }  else  {
      Serial.read();
    }
  }
}

void wifiConfig(String ssid, String password) {
  jsonDoc.clear();

  { // open configs file
    File configs_file = SPIFFS.open(CONFIG_FILE);
    deserializeJson(jsonDoc, configs_file);
    configs_file.close();
  }

  // Update ssid and password
  jsonDoc["ssid"] = ssid;
  jsonDoc["password"] = password;

  { // write configs file
    File configs_file = SPIFFS.open(CONFIG_FILE, FILE_WRITE);
    serializeJson(jsonDoc, configs_file);
    configs_file.close();
  }

  delay(100);
  ESP.restart();
}

/* --------- อินเตอร์รัป แสดงสถานะการเชื่อม wifi ------------- */
void HandySense_init() {
  // Serial.begin(115200);
  EEPROM.begin(4096);

  Wire.begin();
  Wire.setClock(10000);
  rtc.begin();
  Sensor_init();

  pinMode(relay_pin[0], OUTPUT);
  pinMode(relay_pin[1], OUTPUT);
  pinMode(relay_pin[2], OUTPUT);
  pinMode(relay_pin[3], OUTPUT);
  digitalWrite(relay_pin[0], LOW);
  digitalWrite(relay_pin[1], LOW);
  digitalWrite(relay_pin[2], LOW);
  digitalWrite(relay_pin[3], LOW);

  if(!SPIFFS.begin(true)){ // Format if fail
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  Edit_device_wifi();
  File configs_file = SPIFFS.open(CONFIG_FILE);
  if (configs_file && (!configs_file.isDirectory())) {
    deserializeJson(jsonDoc, configs_file);       // คือการรับหรืออ่านข้อมูล jsonDoc
    configs_file.close();
    if (!jsonDoc.isNull()) {                      // ถ้าใน jsonDoc มีค่าแล้ว
        mqtt_server   = jsonDoc["server"].as<String>();
        mqtt_Client   = jsonDoc["client"].as<String>();
        mqtt_password = jsonDoc["pass"].as<String>();
        mqtt_username = jsonDoc["user"].as<String>();
        password      = jsonDoc["password"].as<String>();
        mqtt_port     = jsonDoc["port"].as<String>();
        ssid          = jsonDoc["ssid"].as<String>();
    }
  }
  xTaskCreatePinnedToCore(TaskWifiStatus, "WifiStatus", 4096, NULL, 10, &WifiStatus, 1);
  xTaskCreatePinnedToCore(TaskWaitSerial, "WaitSerial", 8192, NULL, 10, &WaitSerial, 1);
  setAll_config();
}

bool wifi_ready = false;

void HandySense_loop() {
  if (!wifi_ready) {
    return;
  }

  client.loop();
  // delay(1); // Keep other task can run

  unsigned long currentTime = millis();
  if (currentTime - previousTime_Temp_soil >= eventInterval) {
    // Update data
    float newTemp = 0, newSoil = 0;
    Sensor_getTemp(&newTemp);
    Sensor_getHumi(&humidity);
    Sensor_getSoil(&newSoil);

    bool update_to_server = false;
    if (
        (newTemp >= (temp + difference_temp)) || 
        (newTemp <= (temp - difference_temp)) ||
        (newSoil >= (soil + difference_soil)) || 
        (newSoil <= (soil - difference_soil))
    ) {
        update_to_server = true;
    }

    temp = newTemp;
    soil = newSoil;

    // Check relay work
    ControlRelay_Bytimmer();
    ControlRelay_BysoilMinMax();
    ControlRelay_BytempMinMax();

    if (update_to_server) {
        UpdateData_To_Server();
        previousTime_Update_data = millis();
    }

    DEBUG_PRINTLN("");
    DEBUG_PRINT("Temp : ");       DEBUG_PRINT(temp);      DEBUG_PRINT(" C, ");
    DEBUG_PRINT("Hum  : ");       DEBUG_PRINT(humidity);  DEBUG_PRINT(" %RH, ");
    DEBUG_PRINT("Brightness : "); DEBUG_PRINT(lux_44009); DEBUG_PRINT(" Klux, ");
    DEBUG_PRINT("Soil  : ");      DEBUG_PRINT(soil);      DEBUG_PRINTLN(" %");

    previousTime_Temp_soil = currentTime;
  }
  if (currentTime - previousTime_brightness >= eventInterval_brightness) {
    Sensor_getLight(&lux_44009);
    lux_44009 /= 1000.0; // lx to Klux
    previousTime_brightness = currentTime;
  }
  unsigned long currentTime_Update_data = millis();
  if (previousTime_Update_data == 0 || (currentTime_Update_data - previousTime_Update_data >= (eventInterval_publishData))) {
    //check_sendData_toWeb = 1;
    UpdateData_To_Server();
    previousTime_Update_data = currentTime_Update_data;
  }
}

/* --------- Auto Connect Wifi and server and setup value init ------------- */
bool pause_wifi_task = false;

void TaskWifiStatus(void * pvParameters) {
  while (1) {
    if (pause_wifi_task) {
      delay(10);
      continue;
    }

    connectWifiStatus = cannotConnect;
    if (!WiFi.isConnected()) {
      if (ssid.length() > 0) {
        WiFi.begin(ssid.c_str(), password.c_str());   
      }
    }
     
    while (WiFi.status() != WL_CONNECTED) {
      DEBUG_PRINTLN("WIFI Not connect !!!");

      /* -- ESP Reset -- */
      if (millis() - time_restart > INTERVAL_MESSAGE2) { // ผ่านไป 1 ชม. ยังไม่ได้เชื่อมต่อ Wifi ให้ Reset ตัวเอง
        time_restart = millis();
        ESP.restart();
      }
      delay(100);
    }

    connectWifiStatus = wifiConnected;
    client.setServer(mqtt_server.c_str(), mqtt_port.toInt());
    client.setCallback(callback);
    timeClient.begin();

    client.connect(mqtt_Client.c_str(), mqtt_username.c_str(), mqtt_password.c_str());
    delay(100);

    while (!client.connected() ) {
      client.connect(mqtt_Client.c_str(), mqtt_username.c_str(), mqtt_password.c_str());
      DEBUG_PRINTLN("NETPIE2020 can not connect");
      delay(100);
    }

    if (client.connect(mqtt_Client.c_str(), mqtt_username.c_str(), mqtt_password.c_str())) {
      connectWifiStatus = serverConnected;

      DEBUG_PRINTLN("NETPIE2020 connected");
      client.subscribe("@private/#");

      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer, nistTime);
      printLocalTime();
      rtc.write(&timeinfo);
    }
    wifi_ready = true;
    while (WiFi.status() == WL_CONNECTED && client.connected()) { // เชื่อมต่อ wifi แล้ว ไม่ต้องทำอะไรนอกจากส่งค่า
      //UpdateData_To_Server();
      sendStatus_RelaytoWeb();
      send_soilMinMax();
      send_tempMinMax();   
      delay(500);
    }
    wifi_ready = false;
  }
}

/* --------- Auto Connect Serial ------------- */
void TaskWaitSerial(void * WaitSerial) {
  while (1) {
    if (Serial.available())   webSerialJSON();
    delay(500);
  }
}
