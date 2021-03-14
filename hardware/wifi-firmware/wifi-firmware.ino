/**
 * Tested : 15 Januari 2021
 * Author : Lasida Azis
 * Source : https://github.com/gsampallo/esp32cam-gdrive
 * Thanks to Gsampallo
 * 
 * This script will give you output base64image with urlencode format
 * you can decode using urldecode and pase to base64 to imaage, and you will get
 * appearance from your image
 * 
 * VGA Size Image : 21KB / 21.189 bytes
 * WIFI and Camera Probem
 * Currrent Drop, Get Camera Before WIFI
 * 
 * Features ::
 * Vibration Check
 * Sleep
 * Post Image Over Wifi
 * Partial POST HTTP for bigger Size
 * 
 * Module ::
 * ESP32CAM
 * Vibration Module
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include "time.h"
#include <TimeLib.h>
#include <String.h>

#include "esp_camera.h"
#include "esp_timer.h"

#include "soc/soc.h" //disable brownout problems
#include "soc/rtc_cntl_reg.h"  //disable brownout problems

//Analog Driver
#include "driver/adc.h"
#include "driver/rtc_io.h"

// Helper
#include "Base64.h"
#include "Helper.h"

//Connection
#include <WiFi.h>
#include <HTTPClient.h>

//Camera Type
#define CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

//LED & VIBRATION GPIO
#define GPIO_LED 12
#define BUTTON_PIN_BITMASK 0x2
#define GPIO_VIBRATION 2
#define GPIO_FLASH 4

const byte interruptPin = 2;
int numberOfInterrupts = 0;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// Wifi Credentials
const char* ssid = "vyv";
const char* password = "lasida123";

// Object Intialize
WiFiClient client;
HTTPClient http;

// Set Flag Internet Connected
uint32_t chipid;  

bool status_camera = false;
bool device_status_online = false;
bool device_status_capture = false;
bool device_mode = "collect";

//Post Temp
char jsonVision[50000];
char jsonStatus[100];

//DateTime
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 25200;
const int   daylightOffset_sec = 3600;

char device_timenow[20];
char device_datenow[30];
char device_datetime[40];
char device_unique[20];

// Sleep Factor
#define uS_TO_S_FACTOR 1000000ULL
unsigned long times;
unsigned long timeToSleep;

// RTC Data
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR unsigned long uptime_seconds = 0;
unsigned long local_time_seconds;

/*
 * WIFI Init()
 * Auto Restart Module 3s Not Connected
 * Display RSSI and Local IP
 * Auto Check Connected and Reconnect
*/
void WiFiInit() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  int try_connect = 0;
  while(WiFi.waitForConnectResult() != WL_CONNECTED){      
    Serial.print(".");
    if( try_connect > 3 ){
      ESP.restart();
    }
    try_connect++;
    delay(1000);
  }

}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Connected to AP successfully!");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.disconnected.reason);
  Serial.println("Trying to Reconnect");
  WiFi.begin(ssid, password);
}

/**
 * Setup()
 * 
 * Control BrownOut
 * Get Mac Address 
 * Blink Indicator Startup
 * 
 * Vibration Check()
 * 
 * Connect to WIFI
 * setupCamera();
 * 
 * getDateTime();
 * TimeCapture Check()
 * 
 * CaptureImage()
 * POST Partial()
 * Sleep()
*/

void setup()    
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  Serial.begin(57600);
  
  // The chip ID is essentially its MAC address(length: 6 bytes).
  chipid = ESP.getEfuseMac();
 
  // GPIO Setup
  pinMode(GPIO_FLASH, OUTPUT);
  pinMode(BUTTON_PIN_BITMASK, INPUT); 

  // ----- LED BLINK 10 Times ----- //
  ledcSetup(0, 5000, 13);
  ledcAttachPin(GPIO_LED, 0);
  indicator_fast_blink( 6 );

  // --> Connecting WIFI
  WiFi.disconnect(true); 
  delay(0100);
  WiFiInit();
  
  Serial.println("Wait for WiFi... ");
  WiFi.onEvent(WiFiStationConnected, SYSTEM_EVENT_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, SYSTEM_EVENT_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);   

  ESP32_DEVICE_STATUS("collect", "online");
  // --> Setup Camera
  setupCamera();

  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot Number: " + String(bootCount));
  Serial.println("Uptime : " + String(uptime_seconds));

  //Setup Date TIme UTC
  delay(1000);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  setupDateTime();

  Serial.println("ESP32 :: " + String(chipid) +" Setup Done !!!");

  //Print the wakeup reason for ESP32
  //esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);
  print_wakeup_reason();
}


void loop(){
  // Log RunTime
  times = millis();
  int tseconds = times / 1000;
  local_time_seconds = tseconds % 60;
  
  Serial.println("Runtime : " + String(local_time_seconds) );
  Serial.println("Uptime : " + String(uptime_seconds) );

  //------------------------------ Checking Time -----------------------------//
  if( !status_camera ){
      indicator_error();
      ESP.restart();
  }else{
    //timeToSleep = getTimeLeft( timetoDecimal(device_timenow));
    //Serial.print( "Time to Decimal : " ); Serial.print( timeToSleep ); Serial.println( "s" ); 
    timeToSleep = 600 ; // 5 minutes
    
    if( getCameraPicture() ){
      setupSleep(timeToSleep);
    }
  }

  delay(1);
}


//------------------------------- Device Status -------------------------------//
bool ESP32_DEVICE_STATUS( String statue, String mode_device ){
  uptime_seconds += local_time_seconds;
  Serial.println("POST :: Device Status");
  DynamicJsonDocument doc(100);
  doc["chip"] = String(chipid);
  doc["status"] = statue; 
  doc["batt"] = 100;
  doc["mode"] = mode_device;
  doc["runtime"] = String(uptime_seconds);
  serializeJson(doc, jsonStatus);  
  bool rstatus = ESP32_POST_HTTP( "http://como.ap-1.evennode.com/v1/device/status", jsonStatus );
}

//--------------------------------- Camera ---------------------------------//
void setupCamera(){
  Serial.print( "Camera :: Setup...." );
  
  // Camera Config
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_VGA; // 640x480
    config.jpeg_quality = 10;  
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12;  //0-63 lower number means higher quality
    config.fb_count = 1;
  }

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf(".... Failed 0x%x", err); Serial.println(" ");
    delay(1000);
    ESP.restart();
  }else{
    status_camera = true;
    Serial.println( ".... OK " );
  }

  sensor_t * s = esp_camera_sensor_get();
  s->set_vflip(s, 1); // 0 = disable , 1 = enable
}

int errCount = 0;
bool getCameraPicture(){
  if( errCount > 3 ){
    ESP.restart();
  }
  Serial.print("CAM :: Take Photo...");
  
  // Flash ON
  digitalWrite(GPIO_FLASH, HIGH);
  delay(500);

  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();  
  if(!fb) {
    digitalWrite(GPIO_FLASH, LOW);
    Serial.print("... Camera capture failed");
    delay(1000);
    ESP.restart();
  }
  Serial.println("... OK");

  digitalWrite(GPIO_FLASH, LOW);
  delay(500);

  // Encode Base64String
  Serial.print("CAM :: Encode to Base64String...");
  char *input = (char *)fb->buf;
  char output[base64_enc_len(3)];
  String base64Image = "";
  for (int i=0;i<fb->len;i++) {
    base64_encode(output, (input++), 3);
    if (i%3==0) base64Image += urlencode(String(output));
  }
  esp_camera_fb_return(fb);
  Serial.println("... OK");

  // Header Data
  Serial.print("ESP32 :: Generating Payload...");
  int parts = round(base64Image.length() / 3000);
  String payloadID = String(chipid) + '-' + String(device_unique);

  // Vision Partial Sender
  int Index;
  int cIndex = 0;
  for (Index = 0; Index < base64Image.length(); Index = Index+3000) {
    // Populate JSON
    Serial.print("ESP32 :: POST PARTIAL...");
    String chunkVision = base64Image.substring(Index, Index+3000);
    using SpiRamJsonDocument = BasicJsonDocument<SpiRamAllocator>;
    SpiRamJsonDocument doc(1048576);
    doc["id"]= payloadID;
    doc["chip"] = String(chipid);
    doc["vision"] = chunkVision;
    doc["index"] = cIndex;
      
    if( cIndex == parts ){
      doc["parity"] = "true";
      doc["chip"] = String(chipid);
      doc["lat"]  = "-6.1954842666402";
      doc["long"] = "106.63440971165635";
      doc["batt"] = "100";
      doc["mode"] = "collect";
      doc["length"] = base64Image.length();
      doc["parts"] = parts;
      delay(1000);
    }
    doc["chunksize"] = chunkVision.length();
      
    serializeJson(doc, jsonVision);  
    Serial.println("... OK");

    Serial.print("Length Image : "); Serial.println( base64Image.length());

    // Sending Payload
    Serial.print("ESP32 :: Sending Payload...");
    bool rstatus = ESP32_POST_HTTP( "http://como.ap-1.evennode.com/v1/device/data", jsonVision );
    if( rstatus ){
      Serial.println("....OK");
    }else{
      Serial.println("....Failed");
      errCount++;
    }
    cIndex++;
    delay(50);
  }

  // Reset JSON Char
  memset(jsonVision, 0, sizeof(jsonVision));
  
  //Serial.println("PSRAM found: " + String(psramFound()));
  Serial.print("Total heap: ");
  Serial.println(ESP.getHeapSize());
  Serial.print("Free heap: ");
  Serial.println(ESP.getFreeHeap());
  Serial.print("Total PSRAM: ");
  Serial.println(ESP.getPsramSize());
  Serial.print("Free PSRAM: ");
  Serial.println(ESP.getFreePsram());

  return true;
}

//--------------------------------- WIFI HTTP POST ---------------------------------//
bool ESP32_POST_HTTP( char* ENDPOINTS, char* JsonDoc)
{   
  //  btStop();
  delay(1310);
  
  // WIfi Connected -> Send Data to Server
  if(WiFi.status()== WL_CONNECTED){
    unsigned long start = micros();
   
    http.setTimeout(10000);
    http.begin( ENDPOINTS );

    http.addHeader("Connection", "keep-alive");
    http.addHeader("Cache-Control", "no-cache");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept-Encoding", "gzip, deflate");
    http.addHeader("Content-Length", String(sizeof(jsonVision)) );
    int httpCode = http.POST(JsonDoc);

    if( httpCode == 0 || httpCode > 0 ){
      String response = http.getString(); 
      Serial.print("HTTP CODE : ");
      Serial.println(httpCode); 

      Serial.print("RESPONSE : ");
      Serial.println(response);
      
      if( httpCode == 200 ){
         return true;
      }else{
         return false;
      }
    }else{
      Serial.print("Error on sending POST: ");
      Serial.println(httpCode);
      return false;
    }

    http.end();

    unsigned long end = micros();
    unsigned long delta = end - start;
    Serial.print( "WIFI : Sent Time :: "); Serial.print( int(delta / 1000 / 60 / 60)); Serial.println("s");
  }else{
    Serial.println("Error in WiFi connection");   
    return false;
  }
  delay(1);
}

//--------------------------------- Sleeping Setup ---------------------------------//
tmElements_t tm;
void setupSleep( int timeSleep ){

  // Ambil Sisa Detik untuk TIdur dari Waktu Sekarang
  char* monthList[12] = {"Januari", "Februari", "Maret", "April", "Mei", "Juni", "Juli", "Agustus", "September", "Oktober", "November", "Desember" };
  Serial.print( "Time to Sleep : " ); Serial.print( timeSleep ); Serial.println( "s" ); 
  //sendStatus( "Sleep(" + String(timeSleep) + "s)" ); 
  
  time_t nextMakeTime;
  int ttS = timeSleep;
  uint32_t rem = ttS%3600;
  uint32_t addHour = ttS/3600;
  uint32_t addMinute = rem/60;
  uint32_t addSecond = rem%60;

  tm.Hour = getValue( device_timenow, ':', 0 ).toInt() + addHour;
  tm.Minute = getValue( device_timenow, ':', 1 ).toInt() + addMinute;
  tm.Second = 0 + addSecond;
  tm.Day = getValue( device_datenow, '/', 2 ).toInt();
  tm.Month = getValue( device_datenow, '/', 1 ).toInt();
  tm.Year = getValue( device_datenow, '/', 0 ).toInt() - 1970; // offset from 1970;
  nextMakeTime = makeTime(tm); // convert time elements into time_t
  
  Serial.print("Wake Up On : ");   
  Serial.print(day(nextMakeTime));   Serial.print( " " ); Serial.print( monthList[month(nextMakeTime) - 1]); Serial.print( " " ); Serial.print(year(nextMakeTime));   
  Serial.print( " @ " ); Serial.print(hour(nextMakeTime)); Serial.print( ":" ); Serial.print(minute(nextMakeTime)); Serial.print( ":" ); Serial.println(second(nextMakeTime));  
  
  uptime_seconds += local_time_seconds;

  ESP32_DEVICE_STATUS("collect", "sleep");
  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  
  esp_sleep_enable_timer_wakeup(timeSleep * uS_TO_S_FACTOR);
  
  Serial.println("SP32 to sleep for  " + String(timeSleep) + " Seconds");
  Serial.println("Sleep : zZZZzzZZZZzzzZZZ");
  
  indicator_fast_blink( 3 );
  Serial.flush(); 
  esp_deep_sleep_start();
}

//--------------------------------- NetworkTime ---------------------------------//
void setupDateTime(){
  struct tm timeinfo;
  while(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    delay(500);
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

  strftime(device_datenow,30, "%Y/%m/%d", &timeinfo); // 2020-01-25
  strftime(device_timenow,20, "%H:%M:%S", &timeinfo); // 10:24:03
  strftime(device_datetime,40, "%Y/%m/%d, %H:%M:%S", &timeinfo);
  strftime(device_unique,20, "%H%M%S", &timeinfo); // 102403

  Serial.print("NetworkTime :: DateTime = ");
  Serial.println(device_datetime);
  
  Serial.print("NetworkTime :: Date = ");
  Serial.println(device_datenow);
  
  Serial.print("NetworkTime :: Time = ");
  Serial.println(device_timenow);
}

String deviceTimeNow(){
  return device_timenow;
}

String deviceDateNow(){
  return device_datenow;
}
