//Get Value With Delimiter
String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  } 

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

//Writing LED
void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255) {
  // calculate duty, 8191 from 2 ^ 13 - 1
  uint32_t duty = (8191 / valueMax) * min(value, valueMax);
  // write duty to LEDC
  ledcWrite(channel, duty);
}

void indicator_thircle(){
  for( int cTh = 1; cTh < 10; cTh++){
    ledcAnalogWrite(0, 255);
    delay(100);
    ledcAnalogWrite(0, 0);
    delay(100);
  }
}

void indicator_blink(){
   ledcAnalogWrite(0, 255);
    delay(1000);
  ledcAnalogWrite(0, 0);
    delay(1000);
}

void indicator_error(){
  ledcAnalogWrite(0, 255);
  delay(1000);
}

void indicator_fast_blink( int ctimes ){ 
  for( int cTh = 0; cTh < ctimes; cTh++){ // 5 times blink
    ledcAnalogWrite(0, 255);
    delay(100);
    ledcAnalogWrite(0, 0);
    delay(100);
  }
}

//https://github.com/zenmanenergy/ESP8266-Arduino-Examples/
String urlencode(String str)
{
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= '+';
      } else if (isalnum(c)){
        encodedString+=c;
      } else{
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2='\0';
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        //encodedString+=code2;
      }
      yield();
    }
    return encodedString;
}

int timetoDecimal( String strtime )
{
  /*
   * Time to Decimal
   * 00.00 == 0
   * 00.01 == 1
   * 01.00 == 60
   * 02.00 == 120
   * 03.00 == 180
   * 04.00 == 240
   * 05.00 == 300
   * 06.00 == 360
   * 07.00 == 420
   * 08.00 == 480
   * 08.30 == 510 ( 8.15 -> split 8 * 60 + 30 == 1395 )
   * 09.00 == 540
   * 09.30 == 570
   * 10.00 == 600
   * 23.00 == 1380
   * 23.15 == 1395 ( 23.15 -> split 23 * 60 + 15 == 1395 )
   * 23.30 == 1410
   * 24.00 == 1440
   */
  int strhour = getValue( strtime, ':', 0 ).toInt(); // 00
  int strminute = getValue( strtime, ':', 1 ).toInt(); // 00
  return strhour * 60 + strminute;
}

/**
 * Calculate Battery based on Voltage Raw
 */
int calcBatt( int voltraw ){
  float batt;
  float tempBatt[32] = {}; 
  float tempBattRaw[32] = {}; 
  float tempBattVolt[32] = {}; 
  float tempBattReal[32] = {};

  int avgRaw = 0;
  float avgVolt = 0.0;
  float avgRealVolt = 0.0;
  float avgBatt = 0.0;

  for (int i = 0; i < 33; i++)
  { // Get Sample Data for 33 Times

    // Read Raw Voltage ADC from GPIO 13
    tempBattRaw[i] = voltraw; // [3.7,3.6]
    avgRaw = avgRaw + tempBattRaw[i]; // 3.7 + 3.6

    // Calculate Voltage based Sensor
    float voltage = (voltraw * 3.3 ) / (4095) + 0.12 ; // 0.177c is calibration
    tempBattVolt[i] = voltage;
    avgVolt = avgVolt + tempBattVolt[i]; 

    // Calculate Real Voltage 
    float realvolt = voltage * 2;
    tempBattReal[i] = realvolt;
    avgRealVolt = avgRealVolt + tempBattReal[i];  

    // Calculate Capacity with Threshold
    float Vmin = 3.8;
    float Vmax = 4.1;
    batt = ((realvolt-Vmin)/(Vmax-Vmin))*100;
    if (batt > 100)
      batt = 100;
    else if (batt < 0)
      batt = 0;

    tempBatt[i] = batt;
    avgBatt = avgBatt + tempBatt[i]; 
    
    // Smoothing
    if (i == 32)  //33 samples are collected
    {
//       SerialBT.print("Raw = ");
//       SerialBT.println( avgRaw / 33 );
//          
//       SerialBT.print("Voltage = ");
//       SerialBT.println( avgVolt / 33 );
//       
//       SerialBT.print("Real Voltage = ");
//       SerialBT.println( avgRealVolt / 33 );
//       
//       SerialBT.print("Battery = ");
//       SerialBT.println( avgBatt / 33 );
       
//       tempBatt[33] = {};
//       tempBattRaw[33] = {};
//       tempBattVolt[33] = {};
//       tempBattReal[33] = {};
//       i = 0;
//       avgRaw = 0;
//       avgVolt = 0;
//       avgRealVolt = 0;
//       avgBatt = 0;
    }
    delay(33);
  }

  return batt;
}

struct SpiRamAllocator {
  void* allocate(size_t size) {
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
  }

  void deallocate(void* pointer) {
    heap_caps_free(pointer);
  }

  void* reallocate(void* ptr, size_t new_size) {
    return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM);
  }
};

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

/*
Method to print the GPIO that triggered the wakeup
*/
void print_GPIO_wake_up(){
  int GPIO_reason = esp_sleep_get_ext1_wakeup_status();
  Serial.print("GPIO that triggered the wake up: GPIO ");
  Serial.println((log(GPIO_reason))/log(2), 0);
}

int getTimeLeft( int timeDecimal ){
  int timeLeft;
  if( timeDecimal  > 420 && timeDecimal  < 480 ){ // 07.00
     timeLeft  = ( 480 - timeDecimal ) * 60 ; // 231 menit * 60 == (s), 420 == 00.00 -- 07.00
  }else if( timeDecimal > 481 && timeDecimal < 540 ){ // 08.00
     timeLeft = ( 540 - timeDecimal ) * 60 ; 
  }else if( timeDecimal > 541 && timeDecimal < 600 ){ // 09.00
     timeLeft = ( 600 - timeDecimal ) * 60 ;
  }else if( timeDecimal > 601 && timeDecimal < 660 ){ // 10.00
     timeLeft = ( 660 - timeDecimal ) * 60 ;
  }else if( timeDecimal > 661 && timeDecimal < 720 ){ // 11.00
     timeLeft = ( 720 - timeDecimal ) * 60 ;
  }else if( timeDecimal > 721 && timeDecimal < 780 ){ // 12.00
     timeLeft = ( 780 - timeDecimal ) * 60 ;
  }else if( timeDecimal > 781 && timeDecimal < 840 ){ // 13.00
    timeLeft = ( 840 - timeDecimal ) * 60 ;
  }else if( timeDecimal > 841 && timeDecimal < 900 ){ // 14.00
    timeLeft = ( 900 - timeDecimal ) * 60 ;
  }else if( timeDecimal > 901 && timeDecimal < 960 ){ // 15.00
    timeLeft = ( 960 - timeDecimal ) * 60 ;
  }else if( timeDecimal > 961 && timeDecimal < 1120 ){ // 16.00
    timeLeft = ( 1120 - timeDecimal ) * 60 ;
  }else if( timeDecimal > 1121 && timeDecimal < 1180 ){ // 17.00
    timeLeft = ( 1180 - timeDecimal ) * 60 ;
  }else if( timeDecimal > 1181 ){ // 18.00 -- 00.00
      timeLeft = ( ( 1440 - timeDecimal ) + 420 ) * 60 ; // 231 menit * 60 == (s), 420 == 00.00 -- 07.00
  }else{ // 00.00 - 07.00
      timeLeft = ( 480 - timeDecimal ) * 60 ;
  }
  return timeLeft;
} 
