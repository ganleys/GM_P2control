#include <Arduino.h>
#include <SoftwareSerial.h>
//#include "board.h"
#include "gsm.h"

#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "sarray.h"

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);
uint32_t lastReconnectAttempt;
int ControllerStatus;


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

// Your GPRS credentials, if any
const char apn[]  = "giffgaff.com";
const char user[] = "giffgaff";
const char pass[] = "";
const char auth[] = "YourAuthToken";

/*
*   GSM initialization
*/
void gsm_Setup(void){

    lastReconnectAttempt = 0;
    ControllerStatus = HIGH;

    pinMode(GSM_PIN, OUTPUT);
    digitalWrite(GSM_PIN,LOW);

    Serial.println("Starting GPRS modem");

    // Set GSM module baud rate
      // Set GSM module baud rate
    //TinyGsmAutoBaud(SerialAT, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX);
    SerialAT.begin(115600);
    delay(1000);
  

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

  
}

/*
*   GSM loop 
*/
void gsm_loop(){

  int8_t c,i=0;

  if (!mqtt.connected()) {
      
      // Reconnect every 30 seconds
      uint32_t t = millis();
      if (t - lastReconnectAttempt > 30000L) {
          lastReconnectAttempt = t;
          Serial.println("MQTT connecting");
              if (mqttConnect()) {
                  lastReconnectAttempt = 0;
          }
      }
      delay(200);
      return;
  }
  else{
    c = sarray_num_cells();
    if(c>0){
      do{
        sarray_get_cell_datastr(c, &jobj);
        serializeJson(jobj,msgbuffer);
        Serial.print("publishing ");
        Serial.println(msgbuffer);
        mqtt.publish(topicTileData,msgbuffer);  
        delay(200);    
      }while(++i<c);
    }

    mqtt.publish(topicContollerStatus,"controllerstatus" );
    //Serial.println("publishing");

  }

  mqtt.loop();

  //delay(GSM_TASK_DELAY);
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
  Serial.print("Connecting to ");
  Serial.print(broker);

  // Connect to MQTT Broker
  boolean status = mqtt.connect("GsmClientTest",mosquittoUser,mosquittoPass);

  // Or, if you want to authenticate MQTT:
  //boolean status = mqtt.connect("GsmClientName", "mqtt_user", "mqtt_pass");

  if (status == false) {
    Serial.println(" fail");
    return false;
  }
  Serial.println(" success");
  mqtt.publish(topicInit, "Grafmarine Array controller started");
  mqtt.subscribe(topicStatus);
  return mqtt.connected();
}