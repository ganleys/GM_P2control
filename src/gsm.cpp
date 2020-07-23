#include <Arduino.h>
#include <SoftwareSerial.h>
//#include "board.h"
#include "gsm.h"

#include <TinyGsmClient.h>
#include <PubSubClient.h>
//SoftwareSerial(int receivePin, int transmitPin, bool inverse_logic = false, unsigned int buffSize = 64);
//SoftwareSerial SerialAT(16,17,false,256);

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
const char* mosquittoUser = "grafdev";
const char* mosquittoPass = "Solar";

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
    delay(6000);
    //SerialAT.println("test");

    if(!modem.restart()){    //initialize modem
      Serial.println("modem restart error");
    }else{
      Serial.println("modem restarted");
    }    
    
              
    if(!modem.waitForNetwork(60000L)){
      Serial.println("modem no network");
    }else{
      Serial.println("modem network found");      
    }

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
    if (!mqtt.connected()) {
        Serial.println("=== MQTT NOT CONNECTED ===");
        // Reconnect every 10 seconds
        uint32_t t = millis();
        if (t - lastReconnectAttempt > 10000L) {
            lastReconnectAttempt = t;
                if (mqttConnect()) {
                    lastReconnectAttempt = 0;
            }
        }
        delay(100);
        return;
    }

  mqtt.loop();
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