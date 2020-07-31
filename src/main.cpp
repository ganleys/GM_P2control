#include <Arduino.h>
//#include "board.h"
#include <LoRa.h>
#include <WiFi.h>
#include <wire.h>
#include <SPI.h>
//#include <Mcu.h>
#include "gsm.h"
#include "gps.h"
#include "compass.h"
#include "gyro.h"


#define RST   14   // GPIO14 -- SX1278's RESET
#define DIO1  25
uint32_t  LICENSE[4] = {0xDD251C39,0x44C5F937,0xBB160873,0x072EB7C8};
//#define USE_BAND_868
#define BAND 866E6
  
uint64_t chipid;  

void setup() {
  // configure the serial ports
  Serial.begin(115200);
  while (!Serial);
  Serial.println("***************************");
  Serial.println("Grafmarine Array Controller");
  Serial.println("***************************\r\n");
  chipid=ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
	Serial.printf("ESP32 Chip ID = %04X",(uint16_t)(chipid>>32));//print High 2 bytes
	Serial.printf("%08X\n",(uint32_t)chipid);//print Low 4bytes.

  //configure the SPI port and I/O for Lora network use
  //SPI.begin(SCK,MISO,MOSI,SS);
  //Mcu.begin(SS,RST,DIO0,DIO1,LICENSE);

  //configure the I2C
  //Wire.begin();

  //configure the GSM modem
  //gps_setup();

  //compass setup
  comapass_setup();

  //gyro setup
  //gyro_Setup();

  //start the LoRa network interface
  //LoRa.DeviceStateInit(CLASS_A);
  //Serial.println("LoRa Started");

  Serial.println("**************");
  Serial.println("Setup complete");
  Serial.println("**************\r\n");
}

void loop() {

  //gps_loop();
  compass_loop();
  //gyro_loop();
}