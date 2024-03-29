/*
*   Array data management and bus interface.
*
*   sarray.cpp
*/
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <esp_task_wdt.h>
#include <ArduinoJson.h>
#include <time.h>
#include "sarray.h"


SoftwareSerial SBserial;
CELL slave_array[SLAVE_ARRAY_SZ];
uint8_t tx_array[TX_CELL_FRAME_SZ];
uint8_t rx_array[CELL_FRAME_SZ];
uint8_t slave_count;
uint8_t slave_update_count = 0;
uint8_t sarray_scan_now;

StaticJsonDocument<200> tstr; 
bool sarray_dav;
time_t repeat_time;

extern byte array_id;

void sarray_Setup(){

    //start the serial bus interface
    //SBserial.begin(2400);
    SBserial.begin(2400, SWSERIAL_8N1, SBRX, SBTX);
    //pinMode(SBTX,PULLUP);

    //request an array scan
    sarray_scan_now = 1;
    //slave_count = 7;

    //no updates yet
    sarray_dav = false;

    repeat_time = now();

}

/*
*   main processing loop that perodically retrieves data 
*   from the cells in the attached array
*/
void sarray_loop(bool parameter){
    int8_t err;
    time_t t;

    //for(;;){

        t = now();

        if((repeat_time == minute(t)) || (parameter == true)){

            //check top see if a slave scan is needed
            if(sarray_scan_now != 0)
                sarray_scan();    

            do{
                //sarray_dav = false;

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
                    delay(SAARAY_SCAN_DEKAY);

                    err = saary_slv_param_get(slave_array[slave_update_count].address, voltage_reg , &slave_array[slave_update_count].scap);
                    if(err == MSG_OK){
                        Serial.print("Scap V = ");Serial.println(slave_array[slave_update_count].scap);
                    }else{
                        Serial.print("Scap V Error ");Serial.println(err);
                    }      
                    delay(SAARAY_SCAN_DEKAY);

                    err = saary_slv_param_get(slave_array[slave_update_count].address, battery_reg , &slave_array[slave_update_count].battery);
                    if(err == MSG_OK){
                        Serial.print("Scap V = ");Serial.println(slave_array[slave_update_count].scap);
                    }else{
                        Serial.print("Scap V Error ");Serial.println(err);
                    }             
                    delay(SAARAY_SCAN_DEKAY);

                    err = saary_slv_param_get(slave_array[slave_update_count].address, temp_reg , &slave_array[slave_update_count].temp);
                    if(err == MSG_OK){
                        //slave_array[slave_update_count].temp = (uint16_t) meas_temp_calc( (uint32_t) slave_array[slave_update_count].temp);
                        Serial.print("Temp = ");Serial.println(slave_array[slave_update_count].temp);
                    }else{
                        Serial.print("Temp Error ");Serial.println(err);
                    } 
                    
                    slave_array[slave_update_count].updated = true;
                    
                    slave_update_count++;

                    //increment the update counter
                    if(slave_update_count == slave_count){
                        slave_update_count = 0;
                        sarray_scan_now = 1;
                        sarray_dav = true;
                    }
                }
            }while(slave_update_count > 0);

            //calculate the next interval
            repeat_time = minute() + SARRAY_DELAY;
            if(repeat_time >= 60)
                repeat_time = repeat_time - 60;

            Serial.print("sarray next scan at " );
            Serial.print(repeat_time);
        }     
    //}
}

/*
*   scans the array for slaves
*   found slave addresses are found are placed in the slave_array
*   returns number of slaves 0 to SLAVE_ARRAY_SZ
*/
int8_t sarray_scan(){
    uint8_t len = 0;
    uint16_t loop_count = 0;
    uint8_t sum;
    
    //clear the array and reset the number of slaves
    memset(slave_array,0,sizeof(slave_array));

    if(slave_count > 0){
          
        slave_count = 0;
    }
    slave_update_count = 0;
    

    //scan the array
    for(uint8_t i = 0x10; i < 0x18; i++){     //SLAVE_ARRAY_SZ

        memset(tx_array,0,sizeof(tx_array));
        sum = 0;
        //send request to a slav
        tx_array[HOST_ADDRESS_BYTE] = i;
        tx_array[HOST_REGISTER_BYTE] = 0;
        tx_array[HOST_PARAM_BYTE] = 2;
        tx_array[HOST_PARAM_BYTE+1] = 3;
        tx_array[HOST_PARAM_BYTE+2] = 4;
        for (int c = 0; c< 7; c++)
            sum += tx_array[c];
        tx_array[HOST_CSUM_BYTE] = 0xff - sum;     
        Serial.print("Scanning slave ");Serial.print(i);
        memset(rx_array,255,sizeof(rx_array));
        //SBserial.setTimeout(ARRAY_TIMEOUT);
        SBserial.write(tx_array,TX_CELL_FRAME_SZ);

        loop_count = 0;
        
        do{
            len =0;

            if(SBserial.available() >0){
                if(SBserial.read() == i){
                //read the return message
                    len = SBserial.readBytes(&rx_array[1],6);
                    rx_array[0] = i;
                    Serial.print(" rx -");
                    for(int c = 0; c< CELL_FRAME_SZ; c++){
                        Serial.print(" ,");
                        Serial.print(rx_array[c]);
                    }
                    
                    loop_count = 3001;
                }
            }else
                loop_count++;

            delay(1);

        }while(loop_count < 1000);
        
        
        //check the returned message
        if(len >= 4){
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
        //SBserial.flush();
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

    uint8_t sum =0;
    uint16_t loop_count = 0;

    memset(tx_array,0,sizeof(tx_array));
    tx_array[HOST_ADDRESS_BYTE] = address;
    tx_array[HOST_REGISTER_BYTE] = reg;
    tx_array[HOST_REGISTER_BYTE-1] = reg;
    for (int c = 0; c< 7; c++)
        sum += tx_array[c];
    tx_array[HOST_CSUM_BYTE] = 0xff - sum; 

    Serial.print(" Tx -");
    for(int c = 0; c< CELL_FRAME_SZ; c++){
        Serial.print(" ,");
        Serial.print(tx_array[c]);
    }
    Serial.print("----");
    
    SBserial.write(tx_array,TX_CELL_FRAME_SZ);

    do{
        
        if(SBserial.available() >0){
            if(SBserial.read() == address){
            //read the return message
                SBserial.readBytes(&rx_array[1],6);
                rx_array[0] = address;
                Serial.print(" rx -");
                for(int c = 0; c< CELL_FRAME_SZ; c++){
                    Serial.print(" ,");
                    Serial.print(rx_array[c]);
                }
                Serial.print(".....");
                loop_count = 3001;
            }
        }else
            loop_count++;

        delay(1);

    }while(loop_count < 3000);    


    //if(len != CELL_FRAME_SZ)
    //    return 	MSG_BAD_SZ;

    if(rx_array[HOST_ADDRESS_BYTE] != address)
        return 	MSG_BAD_RESPONSE;

    //sum =  sarry_calc_checksum(rx_array);        

    //if(sum != rx_array[HOST_CSUM_BYTE])
    //    return MSG_ERROR;
    
    memcpy(param, &rx_array[HOST_PARAM_BYTE-1], HOST_PARAM_BYTE_SZ);

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

    for(int i=0; i < (HOST_CSUM_BYTE); i++){
        sum += *(array+i);
    }

    sum = 0xff - sum;
    //Serial.println(sum);

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

//  returns the number of cells in an array
int8_t sarray_num_cells(void){
    return (int8_t)slave_count;
}

/*
*creates a json formatted string with the specified cell data
    Input: cell number, pointer to the destination JsonObject, size of the string
    Returns the string length or 0 if cell data not available
*/
int8_t sarray_get_cell_datastr(int8_t cell, JsonObject *jstr){

//char buffer[256]

    char timestr[30];
   
    sprintf(timestr, "%04d%02d%02dT%02d%02d%02dZ", year(),month(),day(),hour(),minute(),second());

    tstr.clear();
    tstr["stamp"] = timestr;
    tstr["array"] = array_id;
    tstr["addr"] = slave_array[cell].address;
    tstr["solar"] = slave_array[cell].solar;
    tstr["scap"] = slave_array[cell].scap;
    tstr["batt"] = slave_array[cell].battery;
    tstr["tempc"] = slave_array[cell].temp;
    tstr["update"] = slave_array[cell].updated;

    *jstr = tstr.as<JsonObject>();

    //serializeJson(jstr,buffer);
    //Serial.print("publishing ");
    //Serial.println(buffer);
    return tstr.size();
}