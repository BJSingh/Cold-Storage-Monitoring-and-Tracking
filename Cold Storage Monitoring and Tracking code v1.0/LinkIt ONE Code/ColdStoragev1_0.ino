#include "PubNub.h"
#include <LGSM.h>

//import all the necessary files for GPRS connectivity
#include "LGPRS.h"
#include "LGPRSClient.h"

#include <LGPS.h>

#include <TH02_dev.h>
#include <Wire.h>
#include "RTClib.h"
#include <LBattery.h>

#include <LFlash.h>
#include <LSD.h>
#include <LStorage.h>

#include <ArduinoJson.h>

RTC_Millis rtc;

#define STORAGE LSD           // Use SD card storage

unsigned long ReportingInterval = 20000;  // How often do you want to update the IOT site in milliseconds (in this case 20 seconds)
unsigned long LastReport = 0;      // When was the last time you reported

unsigned long timestamp;

float temperature=0.0,humidity=0.0;

unsigned int batteryStatus=0;

//define the required keys for using PubNub
char pubkey[] = "pub-c-3754af37-b56d-406b-870c-424b2eba49ea";
char subkey[] = "sub-c-d4cf22c8-bdf2-11e6-b490-02ee2ddab7fe";
char channel[] = "my_channel";

double latitude = 0.00;
double longitude = 0.00;
float altitude = 0.00;
float dop = 100.00;  //Horizontal dilution of position
float geoid = 0.00;  //Height of geoid
float kn_speed = 0.00, kh_speed = 0.00; // Speed in knots and speed in km/h
float track_angle = 0.00;
int fix = 0;
int hour = 0, minute = 0, second = 0;
int sat_num = 0;
int day = 0, month = 0, year = 0;
String time_format = "00:00:00", date_format = "00:00:0000";
String lat_format = "0000.000000", lon_format = "0000.000000";

const char *phoneNum = "";  //Phone number for SMS and emergency alert

String toDate,toTime;

void setup()
{  
  Serial.begin(9600);        // start serial for output
  rtc.begin(DateTime(__DATE__, __TIME__));
  LGPS.powerOn();                  // Start the GPS first as it takes time to get a fix
  STORAGE.begin();
  
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
    
      getGPSData();
      Serial.print("Latitude:");
      Serial.println(latitude,6);
      Serial.print("Longitude:");
      Serial.println(longitude,6);

      publishData();
      subscribeData();

      logdata();
      
      LastReport = millis(); 
      Serial.print("Finish Millis: ");
      Serial.println(millis()); 
  }
  if (LSMS.available())
  {
    readSMS();
    LSMS.flush();
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

  toDate=toDate+String(now.day(), DEC);
  toDate=toDate+String('/');    
  toDate=toDate+String(now.month(), DEC);
  toDate=toDate+String('/');
  toDate=toDate+String(now.year(), DEC);

  toTime=toTime+String(now.hour(), DEC);
  toTime=toTime+String(':');    
  toTime=toTime+String(now.minute(), DEC);
  toTime=toTime+String(':');
  toTime=toTime+String(now.second(), DEC);

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
  root["Temperature"] = temperature;
  root["Humidity"] = humidity;
  root["BatteryLevel"] = batteryStatus;

  if (latitude>1)
  {
  JsonArray& data = root.createNestedArray("latlng");
  data.add(float_with_n_digits(latitude, 6));
  data.add(float_with_n_digits(longitude, 6));
  }

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

byte getGPSData()
{
  gpsSentenceInfoStruct info;
  LGPS.getData(&info);
  if (info.GPGGA[0] == '$')
  {
    String str = (char*)(info.GPGGA);
    str = str.substring(str.indexOf(',') + 1);
    hour = str.substring(0, 2).toInt();
    minute = str.substring(2, 4).toInt();
    second = str.substring(4, 6).toInt();
    time_format = "";
    time_format += hour;
    time_format += ":";
    time_format += minute;
    time_format += ":";
    time_format += second;
    str = str.substring(str.indexOf(',') + 1);
    latitude = convert(str.substring(0, str.indexOf(',')), str.charAt(str.indexOf(',') + 1) == 'S');
    int val = latitude * 1000000;
    String s = String(val);
    lat_format = s.substring(0, (abs(latitude) < 100) ? 2 : 3);
    lat_format += '.';
    lat_format += s.substring((abs(latitude) < 100) ? 2 : 3);
    str = str.substring(str.indexOf(',') + 3);
    longitude = convert(str.substring(0, str.indexOf(',')), str.charAt(str.indexOf(',') + 1) == 'W');
    val = longitude * 1000000;
    s = String(val);
    lon_format = s.substring(0, (abs(longitude) < 100) ? 2 : 3);
    lon_format += '.';
    lon_format += s.substring((abs(longitude) < 100) ? 2 : 3);

    str = str.substring(str.indexOf(',') + 3);
    fix = str.charAt(0) - 48;
    str = str.substring(2);
    sat_num = str.substring(0, 2).toInt();
    str = str.substring(3);
    dop = str.substring(0, str.indexOf(',')).toFloat();
    str = str.substring(str.indexOf(',') + 1);
    altitude = str.substring(0, str.indexOf(',')).toFloat();
    str = str.substring(str.indexOf(',') + 3);
    geoid = str.substring(0, str.indexOf(',')).toFloat();

    if (info.GPRMC[0] == '$')
    {
      str = (char*)(info.GPRMC);
      int comma = 0;
      for (int i = 0; i < 60; ++i)
      {
        if (info.GPRMC[i] == ',')
        {
          comma++;
          if (comma == 7)
          {
            comma = i + 1;
            break;
          }
        }
      }
      str = str.substring(comma);
      kn_speed = str.substring(0, str.indexOf(',')).toFloat();
      kh_speed = kn_speed * 1.852;
      str = str.substring(str.indexOf(',') + 1);
      track_angle = str.substring(0, str.indexOf(',')).toFloat();
      str = str.substring(str.indexOf(',') + 1);
      day = str.substring(0, 2).toInt();
      month = str.substring(2, 4).toInt();
      year = str.substring(4, 6).toInt();
      date_format = "20";
      date_format += year;
      date_format += "-";
      date_format += month;
      date_format += "-";
      date_format += day;
      return sat_num;
    }
  }
  return 0;
}

float convert(String str, boolean dir)
{
  double mm, dd;
  int point = str.indexOf('.');
  dd = str.substring(0, (point - 2)).toFloat();
  mm = str.substring(point - 2).toFloat() / 60.00;
  return (dir ? -1 : 1) * (dd + mm);
}

boolean sendSMS(const char *num, String text)
{
  LSMS.beginSMS(num);
  LSMS.print(text);
  return LSMS.endSMS();
}

void readSMS()
{
  String msg = "";
  String response = "";
  char number[20] = {'\0'};
  LSMS.remoteNumber(number, sizeof(number)); //Get the number of the sender
  while (LSMS.peek() > 0) msg += (char)LSMS.read(); //Get the content of the SMS
  msg.toLowerCase();
  if (msg.indexOf("where") > -1) //Maybe the sender wat to know our location
  {
    response="Cold Storage Monitoring and Tracking Project\n";
    
    if (sat_num > 3) response += "I'm right here: "; //There are enought visible satellites, so the coordinates are attendible
    else response += "My GPS location is \n";
    //Build a link to our position in Google Maps
    response += "http://www.google.com/maps?q=";
    response += lat_format;
    response += ",";
    response += lon_format;
    sendSMS(number, response);
    return;
  }
  return;
}

void logdata(void)
{
    // make a string for assembling the data to log:
    String dataString = "";

    dataString += toDate;
    dataString += ",";
    dataString += toTime;
    dataString += ",";
    dataString += String(latitude,6);
    dataString += ",";
    dataString += String(longitude,6);
    dataString += ",";
    dataString += temperature;
    dataString += ",";
    dataString += humidity;
    dataString += ",";
    dataString += batteryStatus;

    toDate="";
    toTime="";
    
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  LFile dataFile = LSD.open("datalog.csv", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    // print to the serial port too:
    Serial.println(dataString);
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  } 
}

