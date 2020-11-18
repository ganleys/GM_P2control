#include <Arduino.h>
#include <SoftwareSerial.h>
#include <esp_task_wdt.h>
#include <time.h>
//#include "board.h"
#include "gsm.h"


#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "sarray.h"
#include "gps.h"


TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);
uint32_t lastReconnectAttempt;
int ControllerStatus;

extern bool sarray_dav;
extern bool gps_fix;
extern float gps_lat;
extern float gps_lon;


// MQTT details
const char* broker = "grafdev.dnsalias.com";
const char* topicStatus = "GrafmarineController/status";
const char* topicInit = "GrafmarineController/init";
const char* topicContollerStatus = "GrafmarineController/ControllerStatus";
const char* topicTileData = "GrafmarineController/tiledata";
const char* mosquittoUser = "grafdev";
const char* mosquittoPass = "Solar";

const size_t CAPACITY = JSON_OBJECT_SIZE(1);
StaticJsonDocument<CAPACITY> doc;
JsonObject jobj = doc.to<JsonObject>();
char msgbuffer[256];
unsigned char netretry;

// Your GPRS credentials, if any
const char apn[]  = "prepay.tesco-mobile.com";
const char user[] = "tescowap";
const char pass[] = "password";
const char auth[] = "YourAuthToken";

/*
*   GSM initialization
*/
void gsm_Setup(void){

    byte err;
    int hr,min,sec,day,mnth,yr = 0;
    float timezone = 0;
    char timestr[30];

    lastReconnectAttempt = 0;
    ControllerStatus = HIGH;

    pinMode(GSM_PIN, OUTPUT);
    digitalWrite(GSM_PIN,LOW);



    // Set GSM module baud rate
      // Set GSM module baud rate
    //TinyGsmAutoBaud(SerialAT, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX);
    SerialAT.begin(115600);
    delay(1000);
  
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
     

    // MQTT Broker setup
    mqtt.setServer(broker, 1883);
    mqtt.setCallback(mqttCallback);    
    mqtt.setKeepAlive(60000);
    
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

    netretry = 0;
      
}

/*
*   GSM loop 
*/
void gsm_loop(void * parameter){

  int8_t c,i=0;

  //for(;;){

    if (!mqtt.connected()) {
        // Reconnect every 30 seconds
        uint32_t t = millis();
        if (t - lastReconnectAttempt > 30000L) {
            lastReconnectAttempt = t;
            if (mqttConnect()) {
               lastReconnectAttempt = 0;  
               netretry=0;                  
            }else
            {
              netretry++;
            }
            
        }
        delay(200);
        if(netretry > 1){
          digitalWrite(GSM_PIN,HIGH);
          delay(1000);
          gsm_Setup();
        }
    }
    else{

      if(sarray_dav == true){
        c = sarray_num_cells();
        if(c>0){
          i=0;
          do{
            sarray_get_cell_datastr(i, &jobj);
            serializeJson(jobj,msgbuffer);
            Serial.print("publishing ");
            Serial.println(msgbuffer);
            mqtt.publish(topicTileData,msgbuffer);  
            delay(200);
            i++;    
          }while(i<c);
        }

        mqtt.publish(topicContollerStatus,"controllerstatus" );
        //Serial.println("publishing");
        sarray_dav = false;
      }

      if(gps_fix == true){
            gps_get_location(i, &jobj);
            serializeJson(jobj,msgbuffer);
            Serial.print("publishing GPS Location ");
            Serial.println(msgbuffer);
            mqtt.publish(topicTileData,msgbuffer);  
            delay(200);

        gps_fix = false;
      }
      
    }

    mqtt.loop();

    delay(500);
    //vTaskDelay(500);
  //}
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
  Serial.print("\r\nGSM - Connecting to ");
  Serial.print(broker);
  Serial.print("\r\n");

  // Connect to MQTT Broker
  boolean status = mqtt.connect("GsmClientTest",mosquittoUser,mosquittoPass);

  // Or, if you want to authenticate MQTT:
  //boolean status = mqtt.connect("GsmClientName", "mqtt_user", "mqtt_pass");

  if (status == false) {
    state = mqtt.state();
    Serial.print(" fail- error code ");
    switch(state){
      case MQTT_CONNECTION_TIMEOUT :
        Serial.println("MQTT_CONNECTION_TIMEOUT");
        break;
      case  MQTT_CONNECTION_LOST :
        Serial.println("MQTT_CONNECTION_LOST");
        break;
      case MQTT_CONNECT_FAILED  :
        Serial.println("MQTT_CONNECT_FAILED");
        break;
      case MQTT_DISCONNECTED  :
        Serial.println("MQTT_DISCONNECTED");
        break;
      case MQTT_CONNECTED   :
        Serial.println("MQTT_CONNECTED");
        break;
      case MQTT_CONNECT_BAD_PROTOCOL  :
        Serial.println("MQTT_CONNECT_BAD_PROTOCOL");
        break;
      case MQTT_CONNECT_BAD_CLIENT_ID :
        Serial.println("MQTT_CONNECT_BAD_CLIENT_ID");
        break;
      case MQTT_CONNECT_UNAVAILABLE :
        Serial.println("MQTT_CONNECT_UNAVAILABLE");
        break;
      case MQTT_CONNECT_BAD_CREDENTIALS :
        Serial.println("MQTT_CONNECT_BAD_CREDENTIALS");
        break;
      case MQTT_CONNECT_UNAUTHORIZED :
        Serial.println("MQTT_CONNECT_UNAUTHORIZED");
        break;
      default  :      
        Serial.println("MQTT_OTHER");
        break;   
    }
    return false;
  }
  Serial.println(" success");
  mqtt.publish(topicInit, "Grafmarine Array controller started");
  mqtt.subscribe(topicStatus);
  return mqtt.connected();
};