/********************************************
 * GSM Modem 
 *******************************************/
#include <Arduino.h>
//#include "board.h"


#ifndef _GSM_H_
#define _GSM_H_

#define SerialAT Serial2
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 57600
//GSM modem selection
#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

#define GSM_PIN 13

#define GSM_TASK_DELAY  500/portTICK_PERIOD_MS



void gsm_Setup(void);
void gsm_loop(void * parameter);
void mqttCallback(char* topic, byte* payload, unsigned int len);
boolean mqttConnect();

#endif /*_GSM_H_*/