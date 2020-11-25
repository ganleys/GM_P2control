#include <Arduino.h>
#include <esp_task_wdt.h>
//#include "board.h"
#include <LoRa.h>
#include <WiFi.h>
#include <wire.h>
#include <SPI.h>
#include <time.h>
//#include <Mcu.h>
#include <EEPROM.h>
#include "eprom_app.h"
#include "gsm.h"
#include "gps.h"
#include "compass.h"
#include "gyro.h"
#include "sarray.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

TaskHandle_t *pvGSMTask;
TaskHandle_t *pvSarrayTask;

//time_t scan_timer;

#define RST   14   // GPIO14 -- SX1278's RESET
#define DIO1  25
uint32_t  LICENSE[4] = {0xDD251C39,0x44C5F937,0xBB160873,0x072EB7C8};
//#define USE_BAND_868
#define BAND 866E6
  
uint64_t chipid;
bool first_scan; 
byte array_id; 

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
  EEPROM.begin(EEPROM_SIZE_BYTE);
  array_id = EEPROM.read(ADDR_ADDAY_ID);
  if(array_id >253){
    array_id = 0;
    EEPROM.write(ADDR_ADDAY_ID,array_id);
  }
  Serial.print("Array address ");
  Serial.println(array_id);

  // configure the gsm module
  gsm_Setup();

  //configure the I2C
  //Wire.begin();

  //configure the GSM modem
  gps_setup();

  //compass setup
  //comapass_setup();

  //gyro setup
  //gyro_Setup();

  //scan for solar cells in the arry
  sarray_Setup();
  first_scan = true;

  //start the LoRa network interface
  //LoRa.DeviceStateInit(CLASS_A);
  //Serial.println("LoRa Started");

  //start tasks
 // xTaskCreate(gsm_loop,"GSM loop",2000,NULL,1,pvGSMTask);
  //xTaskCreate(sarray_loop,"sarray loop",1000,NULL,1,pvSarrayTask);

  Serial.println("************** Setup complete **************\r\n");
}

void loop() {

  //time_t t;

  for(;;){

    
    //compass_loop();
    //gyro_loop();

    sarray_loop((void *)first_scan);
    gps_loop();
    first_scan = false;
    gsm_loop((void *)NULL);

    TIMERG0.wdt_wprotect=TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdt_feed=1;
    TIMERG0.wdt_wprotect=0;

  }
}