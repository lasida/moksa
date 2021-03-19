#include "BluetoothSerial.h"

//Bluetooth Defined
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

char frame[100];
byte GNSSrunstatus;;
byte Fixstatus;
char UTCdatetime[18];
char latitude[11];
char logitude[11];
char altitude[8];
char speedOTG[6];
char course[6];
byte fixmode;
char HDOP[4];
char PDOP[4];
char VDOP[4];
char satellitesinview[2];
char GNSSsatellitesused[2];
char GLONASSsatellitesused[2];
char cn0max[2];
char HPA[6];
char VPA[6];
float latGPS;
float longGPS;


void setup() {
  // Open SerialBT communications and wait for port to open:
  Serial.begin(19200);
  SerialBT.begin("ECOV-DEV"); //Bluetooth device 

  SerialBT.println("SerialBT begin ok");

  // set the data rate for the SoftwareSerialBT port
  Serial.println("AT");
  delay(1000);
  Serial.println("AT+IPR=19200");
    delay(1000);
      while ( Serial.available() > 0) SerialBT.write(Serial.read());
  Serial.println("AT+CGNSPWR=1");
  delay(10000);
    while ( Serial.available() > 0) SerialBT.write(Serial.read());
}

void loop() { // run over and over
  SerialBT.println("Getting" );
  get_GPS();
  delay(10000);
}


int8_t get_GPS() {

  int8_t counter, answer;
  long previous;

  // First get the NMEA string
  // Clean the input buffer
  while ( Serial.available() > 0) SerialBT.write(Serial.read());
  // request Basic string
  Serial.println("AT+CGNSINF"); //sendATcommand("AT+CGNSINF", "AT+CGNSINF\r\n\r\n", 2000);

  counter = 0;
  answer = 0;
  memset(frame, '\0', sizeof(frame));    // Initialize the string
  previous = millis();
  // this loop waits for the NMEA string
  do {

if (Serial.available() != 0) {
  frame[counter] = Serial.read();
  counter++;
  // check if the desired answer is in the response of the module
  if (strstr(frame, "OK") != NULL)
  {
    answer = 1;
  }
}
// Waits for the asnwer with time out
  }
  while ((answer == 0) && ((millis() - previous) < 2000));
  frame[counter - 3] = '\0';

  // Parses the string
  strtok_single(frame, ": ");
  GNSSrunstatus = atoi(strtok_single(NULL, ","));;// Gets GNSSrunstatus
  Fixstatus = atoi(strtok_single(NULL, ",")); // Gets Fix status
  strcpy(UTCdatetime, strtok_single(NULL, ",")); // Gets UTC date and time
  strcpy(latitude, strtok_single(NULL, ",")); // Gets latitude
  strcpy(logitude, strtok_single(NULL, ",")); // Gets longitude
  strcpy(altitude, strtok_single(NULL, ",")); // Gets MSL altitude
  strcpy(speedOTG, strtok_single(NULL, ",")); // Gets speed over ground
  strcpy(course, strtok_single(NULL, ",")); // Gets course over ground
  fixmode = atoi(strtok_single(NULL, ",")); // Gets Fix Mode
  strtok_single(NULL, ",");
  strcpy(HDOP, strtok_single(NULL, ",")); // Gets HDOP
  strcpy(PDOP, strtok_single(NULL, ",")); // Gets PDOP
  strcpy(VDOP, strtok_single(NULL, ",")); // Gets VDOP
  strtok_single(NULL, ",");
  strcpy(satellitesinview, strtok_single(NULL, ",")); // Gets GNSS Satellites in View
  strcpy(GNSSsatellitesused, strtok_single(NULL, ",")); // Gets GNSS Satellites used
  strcpy(GLONASSsatellitesused, strtok_single(NULL, ",")); // Gets GLONASS Satellites used
  strtok_single(NULL, ",");
  strcpy(cn0max, strtok_single(NULL, ",")); // Gets C/N0 max
  strcpy(HPA, strtok_single(NULL, ",")); // Gets HPA
  strcpy(VPA, strtok_single(NULL, "\r")); // Gets VPA

  //converto stringa in numero per poterla confrontare
  longGPS = atof (logitude); 
  latGPS = atof (latitude);
   
  SerialBT.println("mia float latitudine");
  SerialBT.println(latGPS,6);
  SerialBT.println("mia float latitudine");
  SerialBT.println(longGPS,6);
  SerialBT.println("UTCdatetime");
  SerialBT.println(UTCdatetime);
  SerialBT.println("latitude"); 
  SerialBT.println(latitude);
  SerialBT.println("logitude");
  SerialBT.println(logitude);
   return answer;  
}


/* strtok_fixed - fixed variation of strtok_single */
static char *strtok_single(char *str, char const *delims)
{
static char  *src = NULL;
char  *p,  *ret = 0;
if (str != NULL)
    src = str;
if (src == NULL || *src == '\0')    // Fix 1
    return NULL;
ret = src;                          // Fix 2
if ((p = strpbrk(src, delims)) != NULL)
{
    *p  = 0;
    src = ++p;
}
else
    src += strlen(src);
return ret;
}
