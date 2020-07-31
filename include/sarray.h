/*
*   Array data management and bus interface.
*
*   sarray.h
*/

#define SBserial Serial2
#define SLAVE_ARRAY_SZ  sizeof(uint8_t)

#define CELL_FRAME_SZ	8
#define ARRAY_TIMEOUT   1000

typedef struct {
    uint8_t address;
    uint16_t solar;
    uint16_t scap;
    uint16_t battery;
    uint16_t temp;
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
	HOST_ADDRESS_BYTE = 0,
	HOST_REGISTER_BYTE = 2,
	HOST_PARAM_BYTE = 3,
	HOST_CSUM_BYTE = 7
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
	voltage_boost_on
};

typedef struct{
	uint8_t led:2;
	uint8_t output:1;	//final boost output state
	uint8_t Hstate:1;	//harvester output switch state
	uint8_t Hen:1;		//harvester enable state
	uint8_t na:3;
}CELL_STATUS;

CELL slave_array[SLAVE_ARRAY_SZ];
uint8_t tx_array[CELL_FRAME_SZ];
uint8_t rx_array[CELL_FRAME_SZ];
uint8_t slave_count;

void sarray_Setup(void);
void sarray_loop(void);
int8_t sarray_scan(void);