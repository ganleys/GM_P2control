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
uint8_t slave_update_count;
uint8_t sarray_scan_now;


void sarray_Setup(){

    //start the serial bus interface
    //SBserial.begin(9600);
    SBserial.begin(9600, SWSERIAL_8N1, SBRX, SBTX);
    pinMode(SBTX,PULLUP);

    //request an array scan
    sarray_scan_now = 1;

}

/*
*   main processing loop that perodically retrieves data 
*   from the cells in the attached array
*/
void sarray_loop(){

    //check top see if a slave scan is needed
    if(sarray_scan_now != 0)
        sarray_scan();    

    if(slave_count == 0)
        sarray_scan_now = 1;

/*        
    else
    {
        
        if(slave_update_count <= slave_count ){

            //get the status byte
            saary_slv_status_get(slave_array[slave_update_count].address,  &slave_array[slave_update_count].status);
            //get the parameter data
            saary_slv_param_get(slave_array[slave_update_count].address, solar_reg , &slave_array[slave_update_count].solar);
            saary_slv_param_get(slave_array[slave_update_count].address, voltage_reg , &slave_array[slave_update_count].scap);
            saary_slv_param_get(slave_array[slave_update_count].address, battery_reg , &slave_array[slave_update_count].battery);
            //saary_slv_param_get(slave_array[slave_update_count].address, battery_reg , &slave_array[slave_update_count].temp);
            slave_array[slave_update_count].updated = true;


            //increment the update counter
            if(slave_update_count == slave_count)
                slave_update_count = 0;
            else
                slave_update_count++;

        }
    }
*/    

    if(slave_update_count == 0) 
        vTaskDelay(SAARY_TASK_DELAY);      

}

/*
*   scans the array for slaves
*   found slave addresses are found are placed in the slave_array
*   returns number of slaves 0 to SLAVE_ARRAY_SZ
*/
int8_t sarray_scan(){
    uint8_t len;
    
    //clear the array and reset the number of slaves
    memset(slave_array,0,sizeof(slave_array));
    slave_count = 0;
    slave_update_count = 0;


    //scan the array
    for(uint8_t i = 0; i < SLAVE_ARRAY_SZ; i++){     //SLAVE_ARRAY_SZ
        memset(tx_array,0,sizeof(tx_array));

        //send request to a slave
        tx_array[HOST_ADDRESS_BYTE] = i;
        tx_array[HOST_REGISTER_BYTE] = 0;
        Serial.print("Scanning slave ");Serial.print(i);
        SBserial.write(tx_array,CELL_FRAME_SZ);
        //SBserial.setTimeout(ARRAY_TIMEOUT);

        //read the return message
        len = SBserial.readBytes(rx_array,CELL_FRAME_SZ);
        
        //check the returned message
        if(len == CELL_FRAME_SZ){
            slave_array[slave_count++].address = rx_array[HOST_ADDRESS_BYTE];
            Serial.println("- Found slave");
        }else{
            Serial.print("- Empty - ");
            Serial.println(len);
        }

        delay(2000);
    }
    return len;
}

/*
*   Gets a parameter form a slave devise
*       Input
*       - slave address
*       - register number
*       - pointer to the storage element
*
*       Return
*       - MSG_BAD_SZ if imcomplete or missing return message
*       - MSG_ERROR if csum invalid           
*
*/
int8_t saary_slv_param_get(uint8_t address, uint8_t reg, uint16_t *param){

    uint8_t len, sum;

    memset(tx_array,0,sizeof(tx_array));
    tx_array[HOST_ADDRESS_BYTE] = address;
    tx_array[HOST_REGISTER_BYTE] = reg;
    
    SBserial.write(tx_array,CELL_FRAME_SZ);

    SBserial.setTimeout(ARRAY_TIMEOUT);

    len = SBserial.readBytes(rx_array,CELL_FRAME_SZ);

    if(len != CELL_FRAME_SZ)
        return 	MSG_BAD_SZ;

    sum =  sarry_calc_checksum(rx_array);        

    if(sum != rx_array[HOST_CSUM_BYTE])
        return MSG_ERROR;
    
    memcpy(param, &tx_array[HOST_PARAM_BYTE], HOST_PARAM_BYTE_SZ);

    return MSG_OK;
}

/*
*   gets the status byte from the slave
*       Input
*       - slave address
*       - pointer to the storage element
*
*       Return
*       - MSG_BAD_SZ if imcomplete or missing return message
*       - MSG_ERROR if csum invalid 
*/
int8_t saary_slv_status_get(uint8_t address,  uint8_t *param){

    uint16_t stemp;
    int8_t len;

    len = saary_slv_param_get(address, status_reg, &stemp);

    if(len == MSG_OK)
        *param = (uint8_t)stemp;

    return len;

}

/*
*   calculates the chaksum for the supplied array
*   and returns the value
*/
uint8_t sarry_calc_checksum(uint8_t *array){

    uint8_t sum =0;

    for(int i=0; i < (CELL_FRAME_SZ-1); i++){
        sum += *(array+i);
    }

    sum = 0xff - sum;

    return sum;
}

