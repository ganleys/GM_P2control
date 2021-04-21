#include <Arduino.h>
#include <TinyGPS.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include "gps.h"

SoftwareSerial ss;
//#define ss Serial2

static const uint32_t GPSBaud = 9600;
static const int MAX_SATELLITES = 40;
static const int PAGE_LENGTH = 40;
unsigned long age, date, time, chars = 0;
unsigned short sentences = 0, failed = 0;
static const double LONDON_LAT = 51.508131, LONDON_LON = -0.128002;
int gps_repeat;
bool gps_fix;
float gps_lat;
float gps_lon;
TinyGPS gps;

StaticJsonDocument<200> gps_tstr; 

extern byte array_id;

struct
{
  int elevation;
  bool active;
} sats[MAX_SATELLITES];

void gps_setup()
{
  time_t t;

  //pinMode(32,INPUT);
  digitalWrite(12,LOW);
  t = now();
  gps_repeat = minute(t)-1;
  gps_fix = false;

  ss.begin(9600,SWSERIAL_8N1, 25);//, NULL, false, 254);
  Serial.print("Testing TinyGPS library v. "); 
  Serial.println(TinyGPS::library_version());
  /*
  Serial.println("by Mikal Hart");
  Serial.println();
  Serial.println("Sats HDOP Latitude  Longitude  Fix  Date       Time     Date Alt    Course Speed Card  Distance Course Card  Chars Sentences Checksum");
  Serial.println("          (deg)     (deg)      Age                      Age  (m)    --- from GPS ----  ---- to London  ----  RX    RX        Fail");
  Serial.println("-------------------------------------------------------------------------------------------------------------------------------------");
  delay(1000);    
  */
}

void gps_loop()
{
  time_t t;
  
  t = now();

  //Serial.println("GPS loop");

  if(gps_repeat != hour(t)){

    if(gps_fix == true){
      Serial.println("GPS fixed");
      return;
    }
      

    //Serial.println("GPS scanning for satellites\r");
    //do{
  
      //print_int(gps.satellites(), TinyGPS::GPS_INVALID_SATELLITES, 5);
      //print_int(gps.hdop(), TinyGPS::GPS_INVALID_HDOP, 5);
      gps.f_get_position(&gps_lat, &gps_lon, &age);
      /*print_float(gps_lat, TinyGPS::GPS_INVALID_F_ANGLE, 10, 6);
      print_float(gps_lon, TinyGPS::GPS_INVALID_F_ANGLE, 11, 6);
      print_int(age, TinyGPS::GPS_INVALID_AGE, 5);
      print_date(gps);
      print_float(gps.f_altitude(), TinyGPS::GPS_INVALID_F_ALTITUDE, 7, 2);
      print_float(gps.f_course(), TinyGPS::GPS_INVALID_F_ANGLE, 7, 2);
      print_float(gps.f_speed_kmph(), TinyGPS::GPS_INVALID_F_SPEED, 6, 2);
      print_str(gps.f_course() == TinyGPS::GPS_INVALID_F_ANGLE ? "*** " : TinyGPS::cardinal(gps.f_course()), 6);
      print_int(gps_lat == TinyGPS::GPS_INVALID_F_ANGLE ? 0xFFFFFFFF : (unsigned long)TinyGPS::distance_between(gps_lat, gps_lon, LONDON_LAT, LONDON_LON) / 1000, 0xFFFFFFFF, 9);
      print_float(gps_lat == TinyGPS::GPS_INVALID_F_ANGLE ? TinyGPS::GPS_INVALID_F_ANGLE : TinyGPS::course_to(gps_lat, gps_lon, LONDON_LAT, LONDON_LON), TinyGPS::GPS_INVALID_F_ANGLE, 7, 2);
      print_str(gps_lat == TinyGPS::GPS_INVALID_F_ANGLE ? "*** " : TinyGPS::cardinal(TinyGPS::course_to(gps_lat, gps_lon, LONDON_LAT, LONDON_LON)), 6);

      gps.stats(&chars, &sentences, &failed);
      print_int(chars, 0xFFFFFFFF, 6);
      print_int(sentences, 0xFFFFFFFF, 10);
      print_int(failed, 0xFFFFFFFF, 9); 
      Serial.print("\r");
      */
      smartdelay(1000);
      
    //}while
    if(gps_lat != TinyGPS::GPS_INVALID_F_ANGLE){

      Serial.print("GPS location ");
      print_float(gps_lat, TinyGPS::GPS_INVALID_F_ANGLE, 10, 6);
      print_float(gps_lon, TinyGPS::GPS_INVALID_F_ANGLE, 11, 6);
      print_date(gps);
      Serial.println("\r");
      
      gps_fix = true;

      //calculate the next interval
      gps_repeat = hour();
      if(gps_fix == true){        
        if(gps_repeat >59)
            gps_repeat = 0;
      }
    }
/*    else{
      Serial.print("GPS error ");
      print_float(gps_lat, TinyGPS::GPS_INVALID_F_ANGLE, 10, 6);
      print_float(gps_lon, TinyGPS::GPS_INVALID_F_ANGLE, 11, 6);
      print_date(gps);
      Serial.println("\r");
    }
*/    
  }
}

void smartdelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());     
  } while (millis() - start < ms);
}

int8_t gps_get_location(int8_t cell, JsonObject *jstr){

    char timestr[30];
   
    sprintf(timestr, "%04d%02d%02dT%02d%02d%02dZ", year(),month(),day(),hour(),minute(),second());

    gps_tstr.clear();
    gps_tstr["stamp"] = timestr;
    gps_tstr["array"] = array_id;
    gps_tstr["long"] = gps_lon;
    gps_tstr["lat"] = gps_lat;

    *jstr = gps_tstr.as<JsonObject>();

    return gps_tstr.size();
}

void print_float(float val, float invalid, int len, int prec)
{
  if (val == invalid)
  {
    while (len-- > 1)
      Serial.print('*');
    Serial.print(' ');
  }
  else
  {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i=flen; i<len; ++i)
      Serial.print(' ');
  }
  smartdelay(0);
}

void print_int(unsigned long val, unsigned long invalid, int len)
{
  char sz[32];
  if (val == invalid)
    strcpy(sz, "*******");
  else
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i=strlen(sz); i<len; ++i)
    sz[i] = ' ';
  if (len > 0) 
    sz[len-1] = ' ';
  Serial.print(sz);
  smartdelay(0);
}

void print_date(TinyGPS &gps)
{
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long age;
  char sz[32];

  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  if (age != TinyGPS::GPS_INVALID_AGE){       
    sprintf(sz, "%02d/%02d/%02d %02d:%02d:%02d ",month, day, year, hour, minute, second);
    Serial.print(sz);
    setTime(hour,minute,second,day,month,year);
  }  
  
}

void print_str(const char *str, int len)
{
  int slen = strlen(str);
  for (int i=0; i<len; ++i)
    Serial.print(i<slen ? str[i] : ' ');
  smartdelay(0);
}