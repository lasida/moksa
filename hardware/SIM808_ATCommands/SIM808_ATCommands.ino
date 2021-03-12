#include "BluetoothSerial.h"
#include "String.h"

//Bluetooth Defined
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

//DateTime
String device_timeID = "";
String device_timenow = "";
String device_datenow = "";
String device_datetime = "";

void setup() {
  Serial.begin(57600);
  SerialBT.begin("ECOV-DEV"); //Bluetooth device 
  Serial.println("AT+IPR=57600");
  delay(7000);
//  connectGPRS();
//  
//  sendAT("AT+CCLK?");
//  delay(1000);
//    sendAT("AT+CLTS=1");
//  delay(1000);
//     sendAT("AT+CLTS?");
//  delay(1000);
//       sendAT("AT&W");
//  delay(1000);
      sendAT("AT+CCLK?");
  delay(1000);
}

void connectGPRS() {
  sendAT("AT+CGATT?");
  delay(1000);
  sendAT("AT+CGATT=1");
  delay(1000);
  sendAT("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
  delay(1000);
  sendAT("AT+SAPBR=3,1,\"APN\",\"telkomsel\"");
  //Using Indian Standard Vodafone Sim and so APN is www
  delay(1000);

  sendAT("AT+SAPBR=1,1");
  delay(1000);
  
  sendAT("AT+SAPBR=2,1");
}

void loop() {
  // put your main code here, to run repeatedly:
  sendAT("AT+CCLK?");
  delay(1000);
}

void sendAT( String command ) 
{
  Serial.println(command);
  delay(1000);
  ShowSerialData();
}

void ShowSerialData()
{
//  while(Serial.available()!=0)
//  {
//    SerialBT.write(Serial.read());
//  }
  String dataIn = "";
  String hasil = "";
  int i;
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
