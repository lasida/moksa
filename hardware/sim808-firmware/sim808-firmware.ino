/**
 * ESP32 Camera
 * SIM808
 * 3 Resistor -> Voltage Divider 8.4 -> 2.8v ( 2 cell )
 * 1 LED + Resistor : Indicator
 * Bluetooth for Debugging
 * 
 * Author : Lasida Azis
 * Source : https://github.com/gsampallo/esp32cam-gdrive
 * Thanks to Gsampallo
 * 
 * Features ::
 * GPS Track :: OK
 * Camera Capture :: OK
 * Time Capture :: OK
 * Battery Capacity :: OK
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <TimeLib.h>

#include "esp_camera.h"
#include "esp_timer.h"

#include "soc/soc.h" //disable brownout problems
#include "soc/rtc_cntl_reg.h" //disable brownout problems

//Analog Driver
#include "driver/adc.h"
#include "driver/rtc_io.h"

// Helper
#include "Base64.h"
#include "Helper.h"

//Connection
#include "BluetoothSerial.h"
#include "SIM800L.h"
#include <DFRobot_sim808.h>

// GPRS APN
const char APN[] = "telkomsel";
const char CONTENT_TYPE[] = "application/json";
const char SERVER[] = "http://webhook.site/";
const char DEVICE_STATUS[] = "c98b5f5e-2765-470c-a788-f095697c1070";
const char DEVICE_DATA[] = "c98b5f5e-2765-470c-a788-f095697c1070";

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

//Bluetooth Defined
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

//LED & VIBRATION GPIO
#define GPIO_LED 12
#define BUTTON_PIN_BITMASK 0x2 // GPIO 2^2 = 0x4
#define GPIO_VIBRATION 2
#define GPIO_FLASH 4

// Object Initialize
BluetoothSerial SerialBT;
SIM800L* sim800l;
DFRobot_SIM808 sim808(&Serial);

// Set Flag Internet Connected
uint32_t chipid;  
bool sim_connected = false;
bool sim_disconnected = false;

bool status_gps = false;
bool status_camera = false;
bool status_sim = false;

// GPS and Mode Temp
double device_lat;
double device_long;

int device_battery;
String device_mode;

//Post Temp
char jsonVision[30000];
char jsonStatus[100];

//DateTime
String device_timeID = "";
String device_timenow = "";
String device_datenow = "";
String device_datetime = "";

// Sleep Variable
#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
unsigned long times;
unsigned long timeToSleep;

// RTC Data
RTC_DATA_ATTR int BOOT = 0;
RTC_DATA_ATTR unsigned long rtc_time_seconds = 0;
unsigned long local_time_seconds;

//============================== SETUP ============================== //
void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  
  chipid = ESP.getEfuseMac();// get ChipID;
  Serial.begin(57600); // Set ESP32 BaudRate
  SerialBT.begin("ECOV-DEV"); // Init Bluetooth Device Name; 

  // ----- Startup LED BLINK and Delay 5s ----- //
  ledcSetup(0, 5000, 13);
  ledcAttachPin(GPIO_LED, 0);
  indicator_fast_blink( 5 );
  pinMode(GPIO_FLASH, OUTPUT);
  
  Serial.println("AT+IPR=57600"); // Set SIM808 BaudRate
  delay(1000);
  Serial.println("AT+CFUN=1,1"); 
  delay(2000);
  Serial.println("AT+BTPOWER=0"); // Disable Bluetooth SIM808
  pinMode(GPIO_VIBRATION, INPUT);  

  // ----- Boot Count ----- //
  ++BOOT;
  SerialBT.println("Boot Number: " + String(BOOT));
  SerialBT.println("Runtime : " + String(rtc_time_seconds));

  // ----- Setup SIM808 using SIM800L Library and Disable Sleep Mode via At Command ----- //
  sim800l = new SIM800L((Stream *)&Serial );
  Serial.println("AT+CSCLK=0"); 
  delay(1000);
  gsmSetup();
  
  // --> Setup Camera
  delay(500);
  cameraSetup();

  // --> Setup Battery
  delay(500);
  getBattery();

  SerialBT.println("ESP32 :: " + String(chipid) +" Setup Done !!!");
  SerialBT.println('\n');

  //Print the wakeup reason for ESP32
  print_wakeup_reason();

  //Wake Up using Trigger | Vibration
  print_GPIO_wake_up();
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);

  //cameraCapture();
}


//============================== LOOP ============================== //
int startFlag;
unsigned long startTime = 0;
unsigned long previousTime = 0;
int duration = 100;

void loop(){
  // Log RunTime
  //  times = millis();
  //  int tseconds = times / 1000;
  //  local_time_seconds = tseconds % 60;
  //  SerialBT.println("runtime : " + String(local_time_seconds) );
  //  SerialBT.println("RTC runtime : " + String(rtc_time_seconds) );
  //  
  // GPS Log
  //  while (Serial2.available() >0) {
  //     gps.encode(Serial2.read());
  //  }
  //  if (millis() > 5000 && gps.charsProcessed() < 10)
  //  {
  //    SerialBT.println(F("tidak ada module GPS, cek wiringmu!"));
  //    while(true);
  //  }

  // Online for 3 minutes and status gps false;
  //  if (millis() > 180000 && status_gps == false ){
  //    SerialBT.println("3 Minutes Passed and GPS Failure...");
  //    getPosition();
  //  }

  //************** Get GPS data *******************


  //--------------------------------- Checking Time ---------------------------------//
  if( !status_sim || !status_camera ){
    //indicator_error();
  }else{
    //timeToSleep = getTimeLeft( timetoDecimal(device_timenow) );
    //SerialBT.print( "Time to Sleep : " ); SerialBT.print( timeToSleep ); SerialBT.println( "s" ); 
  }
  
  SerialBT.println("ESP32 :: " + String(chipid) +" Setup Done !!!");
  delay(1000);
}

/**
 * Getting Battery Capacity
 */
void getBattery()
{
  int batt_raw;
  adc2_config_channel_atten( ADC2_CHANNEL_4, ADC_ATTEN_DB_11  );
  esp_err_t r = adc2_get_raw( ADC2_CHANNEL_4, ADC_WIDTH_12Bit, &batt_raw);

  if ( r == ESP_OK ) {
    device_battery = calcBatt( batt_raw );
    SerialBT.print( "Battery Capacity :: " ); SerialBT.print( device_battery ); SerialBT.println( "%" );
  } else if ( r == ESP_ERR_TIMEOUT ) {
    SerialBT.println("ADC2 used by Wi-Fi.\n");
  }
}




/**
 * Getting Position Device
 * Longtitude
 * Latitude
 */
void setupGPS(){
  SerialBT.println( "GPS :: Checking...." );
  getPosition();
}

bool getPosition(){
  if (sim808.getGPS()) {
    SerialBT.print("latitude :");
    SerialBT.println(sim808.GPSdata.lat);
    device_lat = sim808.GPSdata.lat;
    SerialBT.print("longitude :");
    SerialBT.println(sim808.GPSdata.lon);
    device_long = sim808.GPSdata.lon;
    SerialBT.println( "GPS :: OK !!!" );
    sim808.detachGPS();
  }else{
    SerialBT.println("GPS Low Signal");
  }
}

/**
 * Setup GSM
 * Checking Wiring
 * Checking Signal
 * Network Registration
 * Setup APN
 */
bool gsm_wiring = false;
bool gsm_signal = false;
bool gsm_registered = false;

void gsmSetup(){
  // --> Checking Wiring SIM800L
  SerialBT.println(F("SIM808 :: Setup !!!"));

  // --> Checking Module Ready for 5s
  while(!sim800l->isReady() ) {
    indicator_error(); // Set Indicator Error
    SerialBT.println("SIM808 :: Check Wiring.......Not Detected !!!");
    delay(1000);
  }

  SerialBT.println("SIM808 :: Check Wiring.......OK !!!");
  gsm_wiring = true;
  indicator_clear();

  // --> Setup GPS
  if( sim808.attachGPS() ){
    SerialBT.println("SIM808 :: GPS Power Success");
  }else{
    indicator_error(); // Set Indicator Error
    SerialBT.println("SIM808 :: GPS Power Failure");
  }
      
  // Waiting for Signal
  delay(3210);
  indicator_clear();

  // --> Waiting GSM Signal
  while(!sim800l->getSignal()) {
    SerialBT.println("SIM808 :: Signal not detected !!!");
    indicator_error(); // Set Indicator Error
    delay(1000);
  }
  uint8_t signal = sim800l->getSignal();
  indicator_clear();
  
  // --> Signal Strength
  if( signal > 2 && signal < 11 ){
    SerialBT.print("SIM808 :: Signal.......POOR (");
  }else if( signal > 10 && signal < 15 ){
    SerialBT.print("SIM808 :: Signal.......OK (");
  }else if( signal > 15 && signal < 20 ){
    SerialBT.print("SIM808 :: Signal.......Good (");
  }else if( signal > 20 && signal < 30 ){
    SerialBT.print("SIM808 :: Signal.......Excellent (");
  }
  SerialBT.print(signal);
  SerialBT.println(")");
  gsm_signal = true;

  delay(1000);
  // --> Checking Network Registration for 5s
  NetworkRegistration network = sim800l->getRegistrationStatus();
  while(network != REGISTERED_HOME && network != REGISTERED_ROAMING) {
    SerialBT.print("SIM808 :: Network....");
    if( network != REGISTERED_HOME && network != REGISTERED_ROAMING ){
      SerialBT.println("....Not Registered !!!");
      indicator_error(); // Set Indicator Error
      delay(3000);
    }
    delay(1000);
  }
  indicator_clear();
  delay(300);
  SerialBT.println("SIM808 :: Network....Registered !!!");
  gsm_registered = true;

  // --> Setup APN
  while(!sim800l->setupGPRS(APN) ) {
     SerialBT.println("SIM808 :: APN Configuration ...");
     bool success = sim800l->setupGPRS(APN);
     if( !success ){
      SerialBT.println("....Failed !!!");
      indicator_error(); // Set Indicator Error
      delay(3000);
    }
  }
  indicator_clear();
  SerialBT.println("SIM808 :: APN Configuration ...OK !!!");
  status_sim = true;

  networkServer();
  
  if(device_timeID == "A" ){
    networkServer();
  }

  if( int(device_timeID.length()) > 6 ){
    networkServer();
  }
  
  // Check Capture Time
  timeToSleep = getTimeLeft( timetoDecimal(device_timenow) );
  goSleep(timeToSleep);
}

bool gsmConnect(){
  while(!sim800l->connectGPRS() ) {
   SerialBT.print("SIM808 :: GPRS Connecting ...");
   sim_connected = sim800l->connectGPRS(); // Open Bearer
    if( !sim_connected ){
      SerialBT.println("....Failed !!!");
      indicator_error(); // Set Indicator Error
      sim800l->disconnectGPRS(); // Close Bearer
      delay(15000); // waiting 15s
    }
  }
  SerialBT.println("SIM808 :: GPRS Connecting .......OK !!!");  
}

bool gsmDisconnect(){
  SerialBT.print( "SIM800L :: Disconnecting...." );
  for(uint8_t i = 0; i < 5; i++) {
    
    sim_disconnected = sim800l->disconnectGPRS();
    if( sim_disconnected ){
      SerialBT.println("....OK");
      break;
    }else{
      SerialBT.println("....Failed");
      indicator_error();
      delay(2000);
    }
    delay(1000);
  }
}



/**
 * Method :: POST Device Status  
 */
void deviceStatus(){
//  int powmode = sim800l->getPowerMode();
//  SerialBT.println("PowerMode : ");
//  SerialBT.println(powmode);
  
//  SerialBT.println("POST :: Device Status Online");
//  DynamicJsonDocument doc(100);
//  doc["chip"] = String(chipid);
//  doc["status"] = "online"; 
//  doc["battery"] = device_battery;
//  doc["runtime"] = String(rtc_time_seconds);
//  serializeJson(doc, jsonStatus);  
//  serializeJsonPretty(doc, SerialBT);  
////  bool rstatus = GPRS_POST( ENDPOINT_STATUS, jsonStatus, 60000 );
//  
//  if( rstatus ){
//    device_status_online = true;
//  }
}

/**
 * Camera Setup
 * - Camera Config
 * - FrameSize : VGA
 * - Camera Init
 * - Camera Settings
 */
void cameraSetup(){
  SerialBT.print( "Camera :: Setup...." );
  
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
  delay(3000);
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    SerialBT.printf(".... Failed 0x%x", err); SerialBT.println(" ");
    ESP.restart();
  }else{
    status_camera = true;
    SerialBT.println( ".... OK " );
  }
  
  // Camera Settings
  sensor_t * s = esp_camera_sensor_get();
  s->set_hmirror(s, 1); // 0 = disable , 1 = enable
  s->set_vflip(s, 1);   // 0 = disable , 1 = enable
}

bool cameraCapture(){
  SerialBT.print("Camera :: Take Photo...");

  // -----------------------------> Flash ON
  digitalWrite(GPIO_FLASH, HIGH);
  delay(500);

  // Camera Capture
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();  
  if(!fb) {
    SerialBT.print("... Camera capture failed");
    delay(1000);
    cameraCapture();
    ESP.restart();
  }
  
  digitalWrite(GPIO_FLASH, LOW);
  delay(500);
  SerialBT.println("... OK");
  // -----------------------------> Flash OFF

  // Encoding to Base64 String
  SerialBT.print("CAM :: Encode to Base64String...");
  char *input = (char *)fb->buf;
  char output[base64_enc_len(3)];
  String base64Image = "";
  for (int i=0;i<fb->len;i++) {
    base64_encode(output, (input++), 3);
    if (i%3==0) base64Image += urlencode(String(output));
  }
  esp_camera_fb_return(fb);
  SerialBT.println("... OK");

  // Header Data
  Serial.print("ESP32 :: Generating Payload...");
  int parts = round(base64Image.length() / 1024);
  String device_unique = "";
  String payloadID = String(chipid) + '-' + String(device_unique);

  // Vision Partial Sender
  unsigned long start = micros();
  SerialBT.println("SIM800L : Sending ... " );
  int Index;
  int cIndex = 0;
  for (Index = 0; Index < base64Image.length(); Index = Index+1024) {
    
    // Populate JSON
    String chunkVision = base64Image.substring(Index, Index+1024);
    using SpiRamJsonDocument = BasicJsonDocument<SpiRamAllocator>;
    SpiRamJsonDocument doc(1048576);
    doc["id"]= payloadID;
    doc["chip"] = String(chipid);
    doc["vision"] = chunkVision;
    doc["index"] = cIndex;
      
    if( cIndex == parts ){
      doc["parity"] = "true";
      doc["chip"] = String(chipid);
      doc["lat"]  = "-6.52151";
      doc["long"] = "105.52151";
      doc["batt"] = "100";
      doc["mode"] = "charge";
      doc["length"] = base64Image.length();
      doc["parts"] = parts;
    }
    doc["chunksize"] = chunkVision.length();
      
    serializeJson(doc, jsonVision);  

    // Sending Payload
    SerialBT.print("ESP32 :: Sending Payload...");
    bool rstatus = SIM808_POST_HTTP( "http://webhook.site/ef5e531d-48bd-4517-8d78-61137ff2040e", jsonVision );
    if( rstatus ){
      SerialBT.println("....OK");
    }else{
      SerialBT.println("....Failed");
    }
    cIndex++;
  }

  SerialBT.print("Length Image : "); SerialBT.println( base64Image.length());
  
  unsigned long end = micros();
  unsigned long delta = end - start;
  SerialBT.print( "SIM800L : Sent Time :: ");
  SerialBT.print( int(delta / 1000 / 60 / 60));
  SerialBT.println("s");
  
  // Reset JSON Char
  memset(jsonVision, 0, sizeof(jsonVision));
  
  //Serial.println("PSRAM found: " + String(psramFound()));
  SerialBT.print("Total heap: ");
  SerialBT.println(ESP.getHeapSize());
  SerialBT.print("Free heap: ");
  SerialBT.println(ESP.getFreeHeap());
  SerialBT.print("Total PSRAM: ");
  SerialBT.println(ESP.getPsramSize());
  SerialBT.print("Free PSRAM: ");
  SerialBT.println(ESP.getFreePsram());
  
  return true;
}

//--------------------------------- HTTP POST ---------------------------------//
bool SIM808_POST_HTTP( char* ENDPOINTS, char* JsonDoc)
{    
  // Check if connected, if not reset the module and setup the config again
  if(sim_connected) {
    SerialBT.println(F("SIM800L :: GPRS Connected!"));
    uint16_t rc;
    rc = sim800l->doPost(ENDPOINTS, CONTENT_TYPE, JsonDoc, 10000, 10000); // 20s Timeout
    if(rc == 200 || rc == 703 || rc == 705 ) {
      SerialBT.println("SIM800L :: HTTP POST Success");
      return true;
    } else {
      SerialBT.print("SIM800L :: HTTP POST Error = ");
      SerialBT.println(rc);
      return false;
    }
  } else {
    SerialBT.println(F("SIM800L :: GPRS Not Connected !"));
    SerialBT.println(F("SIM800L :: Reset the module."));
    ESP.restart();
  }
}

//--------------------------------- Sleep ---------------------------------//
tmElements_t tm;
void goSleep( int timeSleep ){
  
  char* monthList[12] = {"Januari", "Februari", "Maret", "April", "Mei", "Juni", "Juli", "Agustus", "September", "Oktober", "November", "Desember" };
  SerialBT.print( "Time to Sleep : " ); SerialBT.print( timeSleep ); SerialBT.println( "s" ); 

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
//  tm.Year = getValue( device_datenow, '/', 0 ).toInt() - 1970; // offset from 1970;
  tm.Year = 2021 - 1970;
  nextMakeTime = makeTime(tm); // convert time elements into time_t

  SerialBT.print("Wake Up On : ");   
  SerialBT.print(day(nextMakeTime));   SerialBT.print( " " ); SerialBT.print( monthList[month(nextMakeTime) - 1]); SerialBT.print( " " ); SerialBT.print(year(nextMakeTime));   
  SerialBT.print( " @ " ); SerialBT.print(hour(nextMakeTime)); SerialBT.print( ":" ); SerialBT.print(minute(nextMakeTime)); SerialBT.print( ":" ); SerialBT.println(second(nextMakeTime));  

  rtc_time_seconds += local_time_seconds;
  
  esp_sleep_enable_timer_wakeup(timeSleep * uS_TO_S_FACTOR);
  SerialBT.println("ESP32 Sleep for  " + String(timeSleep) + " Seconds");
  SerialBT.println("Tidurlah, malam terlalu malam");
  SerialBT.println("Tidurlah, pagi terlalu pagi");
  Serial.println("AT+CSCLK=2"); // Sleep SIM808
  
  indicator_fast_blink( 3 );
  Serial.flush(); 
  SerialBT.flush(); 
  esp_deep_sleep_start();
}

//--------------------------------- NetworkTime ---------------------------------//
void networkServer(){
  delay(3000);
  String dataIn = "";
  String hasil = "";
  int i;
  Serial.println("AT+CCLK?"); delay(2000);
  
  while(Serial.available()!=0)
    dataIn += (char)Serial.read();
    for( i=1; i< dataIn.length(); i++){
        if(  dataIn[i] == '"' || (dataIn[i] == 'C')  || (dataIn[i] == 'L')  || (dataIn[i] == 'K')  || (dataIn[i] == ' ') ){
        }else{
            hasil += dataIn[i];
        }
    }

    for (int i = 0; i < hasil.length(); i++) {
      if (hasil.substring(i, i+1) == ",") {
        device_datenow = hasil.substring(0, i);
        device_timenow= hasil.substring(i+1);
        break;
      }
    }

  device_datenow = getValue( getValue(hasil, ',', 0 ), ':', 1);
  device_timenow = getValue( getValue(hasil, ',', 1 ), '+', 0);
  device_datetime = device_datenow + "," + device_timenow;
  device_timeID = getValue(device_timenow, ':', 0 ) + getValue(device_timenow, ':', 1 ) + getValue(device_timenow, ':', 2 );
      
  SerialBT.print("NetworkTime :: DateTime = ");
  SerialBT.println(device_datetime);
  
  SerialBT.print("NetworkTime :: Date = ");
  SerialBT.println(device_datenow);
  
  SerialBT.print("NetworkTime :: Time = ");
  SerialBT.println(device_timenow);
    
  SerialBT.print("NetworkTime :: TimeID = ");
  SerialBT.println(device_timeID);

}
