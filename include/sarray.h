/*
*   Array data management and bus interface.
*
*   sarray.h
*/

#ifndef SARRAY_H
#define SARRAY_H

#define SAARY_TASK_DELAY	portTICK_PERIOD_MS*3000

//#define SBserial Serial2
#define SLAVE_ARRAY_SZ  16	//maximum number of slave devices in the array

#define SBRX 15
#define SBTX 4

#define CELL_FRAME_SZ	8
#define TX_CELL_FRAME_SZ	8
#define ARRAY_TIMEOUT   1000	// 1second time out

typedef struct {
    uint8_t address;
    uint16_t solar;
    uint16_t scap;
    uint16_t battery;
    uint16_t temp;
	uint16_t status;
	bool updated;
}CELL;

typedef struct{
	uint8_t rw:1;
	uint16_t address:15;
}CELL_ADDRESS;

enum{
	HOST_TX_TRUE,
	HOST_TX_FALSE
};

enum{
	HOST_READ,
	HOST_WRITE
};

enum{
	HOST_ADDRESS_BYTE 	= 0,
	HOST_REGISTER_BYTE	= 2,
	HOST_PARAM_BYTE 	= 3,
	HOST_CSUM_BYTE 		= 7
};

enum{
	HOST_ADDRESS_BYTE_SZ = 1,
	HOST_REGISTER_BYTE_SZ = 1,
	HOST_PARAM_BYTE_SZ = 2,
	HOST_CSUM_BYTE_SZ = 1
};

enum{
	MSG_OK = 0,
	MSG_BAD_SZ = -2,
	MSG_BAD_RESPONSE = -3,
	MSG_ERROR = -1
};

enum{
	status_reg = 0,
	led_pwm_reg,
	solar_reg,
	voltage_reg,
	battery_reg,
	led_thresh,
	device_addr,
	voltage_output_on,
	voltage_boost_on,
	temp_reg = 16
};

typedef struct{
	uint8_t led:2;
	uint8_t output:1;	//final boost output state
	uint8_t Hstate:1;	//harvester output switch state
	uint8_t Hen:1;		//harvester enable state
	uint8_t na:3;
}CELL_STATUS;


void sarray_Setup(void);
void sarray_loop(void);
int8_t sarray_scan(void);
int8_t saary_slv_status_get(uint8_t address,  uint8_t *param);
int8_t saary_slv_param_get(uint8_t address, uint8_t reg, uint16_t *param);
uint8_t sarry_calc_checksum(uint8_t *array);
uint32_t meas_temp_calc( uint32_t inval);

int8_t sarray_num_cells(void);
int8_t sarray_get_cell_datastr(int8_t cell, char *str, uint8_t max_len);

#endif //SARRAY_H