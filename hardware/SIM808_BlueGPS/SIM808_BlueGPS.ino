#include <DFRobot_sim808.h>
#include "BluetoothSerial.h"

//Bluetooth Defined
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

DFRobot_SIM808 sim808(&Serial);
BluetoothSerial SerialBT;

void setup() {
  Serial.begin(19200);
  SerialBT.begin("ECOV-DEV"); // Init Bluetooth Device Name;

  //******** Initialize sim808 module *************
  while(!sim808.init()) { 
      delay(1000);
      SerialBT.print("Sim808 init error\r\n");
  }

  //************* Turn on the GPS power************
  if( sim808.attachGPS())
      SerialBT.println("Open the GPS power success");
  else 
      SerialBT.println("Open the GPS power failure");
  
}

int counting = 0;
void loop() {
   //************** Get GPS data *******************
   if (sim808.getGPS()) {
//    SerialBT.print(sim808.GPSdata.year);
//    SerialBT.print("/");
//    SerialBT.print(sim808.GPSdata.month);
//    SerialBT.print("/");
//    SerialBT.print(sim808.GPSdata.day);
//    SerialBT.print(" ");
//    SerialBT.print(sim808.GPSdata.hour);
//    SerialBT.print(":");
//    SerialBT.print(sim808.GPSdata.minute);
//    SerialBT.print(":");
//    SerialBT.print(sim808.GPSdata.second);
//    SerialBT.print(":");
//    SerialBT.println(sim808.GPSdata.centisecond);
    
    SerialBT.print("latitude :");
    SerialBT.println(sim808.GPSdata.lat,6);
    
    SerialBT.print("longitude :");
    SerialBT.println(sim808.GPSdata.lon,6);
 
    
    SerialBT.print("speed_kph :");
    SerialBT.println(sim808.GPSdata.speed_kph);
    SerialBT.print("heading :");
    SerialBT.println(sim808.GPSdata.heading);

    //************* Turn off the GPS power ************
    sim808.detachGPS();
  }else{
    SerialBT.print("GPS Low Signal, Try :: ");  
    SerialBT.println( counting );
  }
  delay(3000);
  counting += 3;
}
