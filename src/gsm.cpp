#include <Arduino.h>
#include <SoftwareSerial.h>
#include <esp_task_wdt.h>
#include <TimeLib.h>
#include "gsm.h"
#include <WiFi.h>
#include <NTPClient.h>
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "sarray.h"
#include "gps.h"

#define SerialMon Serial
#define TINY_GSM_DEBUG SerialMon


#ifdef TINY_GSM_USE_WIFI
  WiFiClient modem;
  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP);
#else
T inyGsmClient client(modem);
#endif
PubSubClient mqtt(modem);


uint32_t lastReconnectAttempt;
int ControllerStatus;

extern bool sarray_dav;
extern bool gps_fix;
extern float gps_lat;
extern float gps_lon;
extern byte array_id;


// MQTT details
const char* broker = "grafdev.dnsalias.com";
const char* topicStatus = "/graf/status/";
const char* topicUpdate = "/graf/update/";
const char* topicInit = "/graf/init/";
const char* topicContollerStatus = "/graf/status/";
const char* topicTileData = "/graf/tiledata/";
const char* topicTileLocation = "/graf/location/";
const char* mosquittoUser = "grafdev";
const char* mosquittoPass = "grafdev";

const size_t CAPACITY = JSON_OBJECT_SIZE(1);
StaticJsonDocument<CAPACITY> doc;
JsonObject jobj = doc.to<JsonObject>();
char msgbuffer[512];
char sram[1024];

unsigned long epochtime;

unsigned char netretry;

// Your GPRS credentials, if any
const char apn[]  = "prepay.tesco-mobile.com";
const char user[] = "tescowap";
const char pass[] = "password";
const char auth[] = "YourAuthToken";
const char hostname[] = "graf_array1";
const char wifiSSID[] = "CANDS";
const char wifiPass[] = "3376156943";


/*
*   GSM initialization
*/
void gsm_Setup(void){


    int hr,min,sec,day,mnth,yr;
    String formattedDate;
    char tString[200];


#ifdef TINY_GSM_USE_WIFI
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifiSSID);

  WiFi.begin(wifiSSID, wifiPass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();

  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
 

Serial.println((timeClient.getFormattedDate()));

  hr = timeClient.getHours();
  min = timeClient.getMinutes();
  sec = timeClient.getSeconds();

  formattedDate = timeClient.getFormattedDate();
  yr = formattedDate.substring(0,4).toInt();
  mnth = formattedDate.substring(5,7).toInt();
  day = formattedDate.substring(8,10).toInt();
  memset(tString,0,sizeof(tString));
  sprintf(tString," Time set to %d:%d:%d %d-%d-%d",hr,min,sec,day,mnth,yr);
  Serial.println(tString);

  setTime(hr,min,sec,day, mnth, yr);
 
#else

    lastReconnectAttempt = 0;
    ControllerStatus = HIGH;

    pinMode(GSM_PIN, OUTPUT);
    digitalWrite(GSM_PIN,LOW);

    // Set GSM module baud rate
    //SerialAT.begin(115600);
    TinyGsmAutoBaud(SerialAT, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX);
    delay(3000);
  
    Serial.println("Starting GPRS modem");

    do{   //initialize modem
        Serial.println("modem starting");
    }while(!modem.restart());
    Serial.println("modem restarted");

              
    do{
      Serial.println("modem finding network");
    }while(!modem.waitForNetwork(60000L));
    Serial.println("modem network found");      
    

    if(!modem.gprsConnect(apn)){
      Serial.println("modem connect failed");
    }
    else{
      Serial.println("modem connected");
    }

    if (modem.isGprsConnected()) {
        Serial.println("GPRS connected");
    } else
    {
      Serial.println("GPRS NOT connected");
    }
#endif     
    // MQTT Broker setup
    mqtt.setServer(broker, 1883);
    mqtt.setCallback(mqttCallback);  
    mqtt.setBufferSize(512);
    //mqtt.setKeepAlive(120);
    mqtt.subscribe(topicUpdate);
    
    #ifdef TINY_GSM_USE_GPRS
  
      Serial.print("GSM NTP Time - ");
      err = modem.NTPServerSync("pool.ntp.org",0);
    

    if(err <0){
      Serial.print("error "); 
      Serial.println(err);
    }else{
      Serial.print("synced "); 

      modem.getNetworkTime(&yr,&mnth,&day,&hr,&min,&sec,&timezone);
      setTime(hr,min,sec,day,mnth,yr);   
      sprintf(timestr, "%04d%02d%02dT%02d%02d%02dZ", yr,mnth,day,hr,min,sec);

      Serial.println(timestr) ;
    }
    #endif
    netretry = 0;
      
}

/*
*   GSM loop 
*/
void gsm_loop(void * parameter){

  int8_t c,i=0;
  uint16_t counter = 0;
  size_t sz;


  //for(;;){
    timeClient.update();

    if (!mqtt.connected()) {
        // Reconnect every 30 seconds
        uint32_t t = millis();
        if (t - lastReconnectAttempt > 5000L) {
            lastReconnectAttempt = t;
            if (mqttConnect()) {
               lastReconnectAttempt = 0;  
               netretry=0;                  
            }else
            {
              netretry++;
            }
            
        }
        //delay(100);
        if(netretry > 1){
          digitalWrite(GSM_PIN,HIGH);
          delay(2000);
          gsm_Setup();
#ifdef TINY_GSM_USE_GPRS
          if(!modem.gprsConnect(apn)){
            return;
          }

          if (modem.isGprsConnected()) {
              Serial.println("GPRS connected");
          } 
          else
          {
            Serial.println("GPRS NOT connected");
            return;
          }
#endif

          // MQTT Broker setup
          mqtt.setServer(broker, 1883);
          mqtt.setCallback(mqttCallback);  
          mqtt.setBufferSize(512);
          //mqtt.setKeepAlive(120);
          mqtt.subscribe(topicUpdate);

        }
    }
    else{

      if(sarray_dav == true){
        c = sarray_num_cells();
        if(c>0){
          i=0;
          do{
            counter = 0;
            sarray_get_cell_datastr(i, &jobj);
            memset(msgbuffer,0,sizeof(msgbuffer));
            serializeJson(jobj,msgbuffer);
            Serial.print("publishing ");
            Serial.print(topicTileData);
            Serial.println(msgbuffer);
            sz = strlen(msgbuffer);
            if(mqtt.publish(topicTileData,msgbuffer,sz) == false){
              Serial.println("GSM mqtt pub error!");  
              Serial.println(mqtt.getWriteError()); 
            }else{ 
              do{    
                mqtt.loop();    
                delay(10);
                counter++;
              }while(counter < 10);              
            }
            i++;  
            //mqtt.publish(topicContollerStatus,"controllerstatus" );
            //Serial.println("publishing");              
          }while(i<c);
          counter = 0;
        }

        sarray_dav = false;

      }

      if(gps_fix == true){
        counter = 0;
        memset(msgbuffer,0,sizeof(msgbuffer));
        gps_get_location(i, &jobj);
        serializeJson(jobj,msgbuffer);
        Serial.print("publishing GPS Location ");
        Serial.print(topicTileLocation);
        Serial.println(msgbuffer);
        sz = strlen(msgbuffer);       
        if(mqtt.publish(topicTileLocation,msgbuffer,false) == false){
          Serial.print("GSM mqtt pub error ");   
          Serial.println(mqtt.getWriteError());
        }else{
          do{    
            mqtt.loop();    
            delay(100);
            counter++;
          }while(counter < 10);
        }
        counter = 0;
        gps_fix = false;
      }
    }
      
    

    mqtt.loop();
    delay(100);
}

/*
*   mqtt callback function
*/
void mqttCallback(char* topic, byte* payload, unsigned int len) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.write(payload, len);
  Serial.println();

  // Only proceed if incoming message's topic matches
  if (String(topic) == topicStatus) {

    mqtt.publish(topicContollerStatus, ControllerStatus ? "1" : "0");
  }
}

/*
*   mqtt connection function
*/
boolean mqttConnect() {

  int state;
  boolean status;
  Serial.print("\r\nGSM - Connecting to ");
  Serial.print(broker);
  Serial.print("\r\n");

  // Connect to MQTT Broker

      status = mqtt.connect("GrafTest",mosquittoUser,mosquittoPass);

      if (status == false) {
        state = mqtt.state();
        Serial.print(" fail- error code ");
        switch(state){
          case MQTT_CONNECTION_TIMEOUT : Serial.println("MQTT_CONNECTION_TIMEOUT"); break;
          case  MQTT_CONNECTION_LOST :Serial.println("MQTT_CONNECTION_LOST");break;
          case MQTT_CONNECT_FAILED  :Serial.println("MQTT_CONNECT_FAILED");break;
          case MQTT_DISCONNECTED  :Serial.println("MQTT_DISCONNECTED");break;
          case MQTT_CONNECTED   :Serial.println("MQTT_CONNECTED");break;
          case MQTT_CONNECT_BAD_PROTOCOL  :Serial.println("MQTT_CONNECT_BAD_PROTOCOL");break;
          case MQTT_CONNECT_BAD_CLIENT_ID :Serial.println("MQTT_CONNECT_BAD_CLIENT_ID");break;
          case MQTT_CONNECT_UNAVAILABLE :Serial.println("MQTT_CONNECT_UNAVAILABLE");break;
          case MQTT_CONNECT_BAD_CREDENTIALS :Serial.println("MQTT_CONNECT_BAD_CREDENTIALS");break;
          case MQTT_CONNECT_UNAUTHORIZED :Serial.println("MQTT_CONNECT_UNAUTHORIZED");break;
          default  : Serial.println("MQTT_OTHER");break;   
        }
        return mqtt.connected();;
      }
      Serial.println(" success");
      mqtt.publish(topicInit, "Grafmarine Array controller started");
      mqtt.subscribe(topicStatus);
      return mqtt.connected();

};