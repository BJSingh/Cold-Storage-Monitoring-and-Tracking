#include "PubNub.h"

//import all the necessary files for GPRS connectivity
#include "LGPRS.h"
#include "LGPRSClient.h"

#include <LGPS.h>

#include <TH02_dev.h>
#include <Wire.h>
#include "RTClib.h"
#include <LBattery.h>

#include <ArduinoJson.h>

RTC_Millis rtc;

unsigned long ReportingInterval = 20000;  // How often do you want to update the IOT site in milliseconds (in this case 20 seconds)
unsigned long LastReport = 0;      // When was the last time you reported

unsigned long timestamp;

float temperature=0.0,humidity=0.0;

unsigned int batteryStatus=0;

//define the required keys for using PubNub
char pubkey[] = "pub-c-3754af37-b56d-406b-870c-424b2eba49ea";
char subkey[] = "sub-c-d4cf22c8-bdf2-11e6-b490-02ee2ddab7fe";
char channel[] = "my_channel";

void setup()
{  
  Serial.begin(9600);        // start serial for output
  rtc.begin(DateTime(__DATE__, __TIME__));
  
  delay(150);
  /* Reset HP20x_dev */
  TH02.begin();
  delay(100);
  
  /* Determine TH02_dev is available or not */
  Serial.println("##################################################################################");
  Serial.println("Cold Storage Monitoring");   
  
  //Connect to the GRPS network in order to send/receive PubNub messages
  Serial.println("Attach to GPRS network with correct APN settings from your mobile network provider");
  //example here is with mobile provider EE in the UK
  //attachGPRS(const char *apn, const char *username, const char *password);
  
   while (!LGPRS.attachGPRS("http://airtelgprs.com", "", "")) 
  {
  Serial.println(" . ");
  delay(1000);
  }
  Serial.println("LGPRS setup");

  PubNub.begin(pubkey, subkey);
  Serial.println("PubNub setup");
}
 

void loop()
{
    if (millis() >= LastReport + ReportingInterval)   //Send data after about ReportingInterval (i.e.20 seconds)
  {
      Serial.println("#####################################################################################");
      Serial.print("Start Millis: ");
      Serial.println(millis());
      readSensors();  
      publishData();
      subscribeData();
      LastReport = millis(); 
      Serial.print("Finish Millis: ");
      Serial.println(millis()); 
  }

}

void readSensors(void)
{
  DateTime now = rtc.now();
  
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  delay(3000);
  Serial.print(" seconds since 1970: ");
  Serial.println(now.unixtime());
  timestamp = now.unixtime();
  
 temperature = TH02.ReadTemperature(); 
 Serial.println("Temperature: ");   
 Serial.print(temperature);
 Serial.println("C\r\n");
 
 humidity = TH02.ReadHumidity();
 Serial.println("Humidity: ");
 Serial.print(humidity);
 Serial.println("%\r\n");
 delay(1000);

 batteryStatus=LBattery.level();
 Serial.print("Battery LeveL: ");
 Serial.println(batteryStatus);
}

void publishData(void)
{
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["Data"] = "Cold Storage Monitoring";

  JsonArray& data = root.createNestedArray("latlng");
  data.add(float_with_n_digits(48.756080, 6));
  data.add(float_with_n_digits(2.302038, 6));
  
  root["Temperature"] = temperature;
  root["Humidity"] = humidity;
  root["BatteryLevel"] = batteryStatus;
  root["TimeStamp"] = timestamp;
  
  root.prettyPrintTo(Serial);
  Serial.println();
  
  String upload_GPS;
  root.printTo(upload_GPS);

  const char* upload_char = upload_GPS.c_str();
     
  //Once Position is Aquired, upload it to PubNub
  LGPRSClient *client;
  
  Serial.println("publishing a message");
  client = PubNub.publish(channel, upload_char, 60);
  if (!client) {
      Serial.println("publishing error");
      delay(1000);
      return;
  }

  String str="";
  while (client->connected()) {
      while (client->connected() && !client->available()); // wait
      char c = client->read();
      str=str+c;
      //Serial.print(c);
  }
  Serial.print("String: ");
  Serial.println(str);
  client->stop();
  Serial.println();
}

void subscribeData(void)
{
  Serial.println("waiting for a message (subscribe)");
  PubSubClient *pclient = PubNub.subscribe(channel);
  if (!pclient) {
      Serial.println("subscription error");
      delay(1000);
      return;
  }
  while (pclient->wait_for_data()) {
      char c = pclient->read();
      Serial.print(c);
  }
  pclient->stop();
  Serial.println();
}

