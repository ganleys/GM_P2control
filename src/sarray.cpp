/*
*   Array data management and bus interface.
*
*   sarray.cpp
*/
#include <Arduino.h>
#include "sarray.h"
#include <SoftwareSerial.h>

SoftwareSerial SBserial;

CELL slave_array[SLAVE_ARRAY_SZ];
uint8_t tx_array[CELL_FRAME_SZ];
uint8_t rx_array[CELL_FRAME_SZ];
uint8_t slave_count;


void sarray_Setup(){

    //start the serial bus interface
    //SBserial.begin(9600);
    SBserial.begin(2400, SWSERIAL_8N1, SBRX, SBTX);
    pinMode(SBTX,PULLUP);

    //clear the array and reset the number of slaves
    memset(slave_array,0,sizeof(slave_array));
    slave_count = 0;

    

}

/*
*   main processing loop that perodically retrieves data 
*   from the cells in the attached array
*/
void sarray_loop(){

    sarray_scan();    

}

/*
*   scans the array for slaves
*   found slave addresses are found are placed in the slave_array
*   returns number of slaves 0 - 254
*/
int8_t sarray_scan(){
    uint8_t len;

    memset(tx_array,0,sizeof(tx_array));

    for(uint8_t i = 0; i < 250; i++){     //SLAVE_ARRAY_SZ
        tx_array[HOST_ADDRESS_BYTE] = 1;
        tx_array[HOST_REGISTER_BYTE] = 0;
        Serial.print("Scanning slave ");Serial.print(i);
        SBserial.write(tx_array,CELL_FRAME_SZ);
        //SBserial.setTimeout(ARRAY_TIMEOUT);

        
        len = SBserial.readBytes(rx_array,CELL_FRAME_SZ);
        delay(2000);

        
        if(len == CELL_FRAME_SZ){
            slave_array[slave_count++].address = rx_array[HOST_ADDRESS_BYTE];
            Serial.println("Found slave");
        }else{
            Serial.println("Empty");
        }
    }

}

