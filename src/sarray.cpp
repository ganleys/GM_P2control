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
uint8_t tx_array[TX_CELL_FRAME_SZ];
uint8_t rx_array[CELL_FRAME_SZ];
uint8_t slave_count;
uint8_t slave_update_count = 0;
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

    int8_t err;

    //check top see if a slave scan is needed
    if(sarray_scan_now != 0)
        sarray_scan();    

    if(slave_count > 0)
    {
        
        if(slave_update_count < slave_count ){

            //get the status byte
            //saary_slv_status_get(slave_array[slave_update_count].address,  (uint8_t *)&slave_array[slave_update_count].status);
            //get the parameter data
            err = saary_slv_param_get(slave_array[slave_update_count].address, solar_reg , &slave_array[slave_update_count].solar);
            if(err == MSG_OK){
                Serial.print("Solar V = ");Serial.println(slave_array[slave_update_count].solar);
            }else{
                Serial.print("Solar V Error ");Serial.println(err);
            }

            err = saary_slv_param_get(slave_array[slave_update_count].address, voltage_reg , &slave_array[slave_update_count].scap);
            if(err == MSG_OK){
                Serial.print("Scap V = ");Serial.println(slave_array[slave_update_count].scap);
            }else{
                Serial.print("Scap V Error ");Serial.println(err);
            }      

            err = saary_slv_param_get(slave_array[slave_update_count].address, battery_reg , &slave_array[slave_update_count].battery);
            if(err == MSG_OK){
                Serial.print("Scap V = ");Serial.println(slave_array[slave_update_count].scap);
            }else{
                Serial.print("Scap V Error ");Serial.println(err);
            }             
            
            err = saary_slv_param_get(slave_array[slave_update_count].address, temp_reg , &slave_array[slave_update_count].temp);
            if(err == MSG_OK){
                //slave_array[slave_update_count].temp = (uint16_t) meas_temp_calc( (uint32_t) slave_array[slave_update_count].temp);
                Serial.print("Temp = ");Serial.println(slave_array[slave_update_count].temp);
            }else{
                Serial.print("Temp Error ");Serial.println(err);
            } 
            slave_array[slave_update_count].updated = true;

            //increment the update counter
            if(++slave_update_count == slave_count)
                slave_update_count = 0;
        }
    }
    

    if(slave_update_count == 0) 
        vTaskDelay(SAARY_TASK_DELAY); 
    //else  
    //    vTaskDelay(1000);    

}

/*
*   scans the array for slaves
*   found slave addresses are found are placed in the slave_array
*   returns number of slaves 0 to SLAVE_ARRAY_SZ
*/
int8_t sarray_scan(){
    uint8_t len = 0;
    uint16_t loop_count = 0;
    
    //clear the array and reset the number of slaves
    memset(slave_array,0,sizeof(slave_array));

    if(slave_count > 0){
        
    }
    slave_count = 0;
    slave_update_count = 0;


    //scan the array
    for(uint8_t i = 1; i < 7; i++){     //SLAVE_ARRAY_SZ
        memset(tx_array,0,sizeof(tx_array));

        //send request to a slav
        tx_array[HOST_ADDRESS_BYTE] = i;
        tx_array[HOST_REGISTER_BYTE] = 0;
        tx_array[HOST_PARAM_BYTE] = 2;
        tx_array[HOST_PARAM_BYTE+1] = 3;
        tx_array[HOST_PARAM_BYTE+2] = 4;
        Serial.print("Scanning slave ");Serial.print(i);
        //SBserial.setTimeout(ARRAY_TIMEOUT);
        SBserial.write(tx_array,TX_CELL_FRAME_SZ);

        loop_count = 0;
        do{
            delay(1);

            if(SBserial.available() == 8){
            //read the return message
                len = SBserial.readBytes(rx_array,CELL_FRAME_SZ);
/*
                Serial.print(" rx -");
                for(int c = 0; c< len; c++){
                    Serial.print(" ,");
                    Serial.print(rx_array[c]);
                }
*/                
                loop_count = 1001;
            }

            loop_count++;
        }while(loop_count < 1000);
        
        
        //check the returned message
        if(len == CELL_FRAME_SZ){
            if(rx_array[HOST_ADDRESS_BYTE]>250){
                Serial.println(" not found!");            
            }
            else{
                slave_array[slave_count++].address = rx_array[HOST_ADDRESS_BYTE];
                Serial.println("- Found slave");
                sarray_scan_now = 0;
            }
        }else{
            Serial.print("- Empty - ");
            Serial.println(len);
        }
        len = 0;
        delay(2000);
    }
    Serial.print(slave_count); Serial.println(" slaves Found");
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
    
    SBserial.write(tx_array,TX_CELL_FRAME_SZ);
    delay(10);

    len = SBserial.readBytes(rx_array,CELL_FRAME_SZ);

    Serial.print(" rx -");
    for(int c = 0; c< len; c++){
        Serial.print(" ,");
        Serial.print(rx_array[c]);
    }

    if(len != CELL_FRAME_SZ)
        return 	MSG_BAD_SZ;

    if(rx_array[HOST_ADDRESS_BYTE] != address)
        return 	MSG_BAD_RESPONSE;

    sum =  sarry_calc_checksum(rx_array);        

    if(sum != rx_array[HOST_CSUM_BYTE])
        return MSG_ERROR;
    
    memcpy(param, &rx_array[HOST_PARAM_BYTE], HOST_PARAM_BYTE_SZ);

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

//converts adc results into temperature readings
uint32_t meas_temp_calc( uint32_t inval){

	float r, k;

	//1. calculate the resistance
	r = (3103/(float)inval);
	r = (10000*r);

	//2. calculate the temperature
	k = 1/(0.003355705 + (log(10000/r)/3984));
    k *= 100;

	return (int32_t)k-273;

}