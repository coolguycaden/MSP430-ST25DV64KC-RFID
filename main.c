#include "ST25DV64KC.h"


/******************************************************************************
 * Globals
 ******************************************************************************/
// Size of I2C message
#define TX_BUFFER_SIZE 11        

// ID of each node
#define NODE_ID 1 


/******************************************************************************
 * Variable Declaration
 ******************************************************************************/

// Initialize TX buffer
uint8_t TX_buffer[TX_BUFFER_SIZE] = {0};     

int main(void) {

	//Disable watchdog
	WDTCTL = WDTPW | WDTHOLD;

	// Disable the GPIO power-on default high-impedance mode to activate
	// previously configured port settings
	PM5CTL0 &= ~LOCKLPM5;

	//Configure USCI_B2 for I2C Mode
	initializeI2C(); 
	
	volatile unsigned char dataLength = 16;
	volatile unsigned char TX_buffer[dataLength];
    
    TX_buffer[0] = RFID_TAG_STARTING_WRITE_ADDRESS_MSB;
    TX_buffer[1] = RFID_TAG_STARTING_WRITE_ADDRESS_LSB;
    TX_buffer[2] = 0xAA;
    TX_buffer[3] = 0x03;
    TX_buffer[4] = 0x04; 
    TX_buffer[5] = 0x05;
    TX_buffer[6] = 0x06;
    TX_buffer[7] = 0x07;
    TX_buffer[8] = 0x08;
    TX_buffer[9] = 0x09;
    TX_buffer[10] = 0x10;
    TX_buffer[11] = 0x11;
    TX_buffer[12] = 0x12;
    TX_buffer[13] = 0x13;
    TX_buffer[14] = 0x14;
    TX_buffer[15] = 0x15;

    while(1) {
        TX_buffer[0] = RFID_TAG_STARTING_WRITE_ADDRESS_MSB;
        TX_buffer[1] = RFID_TAG_STARTING_WRITE_ADDRESS_LSB;
        TX_buffer[2] = NODE_ID;
        TX_buffer[3] = cur_lux & 0xFF;
        TX_buffer[4] = (cur_lux >> 8) & 0xFF;
        TX_buffer[5] = cur_temp & 0xFF;
        TX_buffer[6] = (cur_temp >> 8) & 0xFF;
        TX_buffer[7] = prior_lux & 0xFF;
        TX_buffer[8] = ((prior_lux) >> 8) & 0xFF;
        TX_buffer[9] = prior_temp & 0xFF;
        TX_buffer[10] = ((prior_temp) >> 8) & 0xFF;

        sendMessage(TX_buffer, &dataLength);
    }

}


unsigned char dataLength = TX_BUFFER_SIZE;
	while(1) 
	{
		go_to_sleep(3);  // Sleep till SENSE_INTERVAL ends
		
		if(timer_B0_interrupt_flag) 
		{
			//0. Collect sensor data
			initialize_internal_temperature_sensor();
			cur_temp = read_temperature();

			initialize_lux_sensor_adc_module();
			cur_lux = get_lux_adc_value();

			//1. Construct Pkt
			TX_buffer[0] = RFID_TAG_STARTING_WRITE_ADDRESS_MSB;
			TX_buffer[1] = RFID_TAG_STARTING_WRITE_ADDRESS_LSB;
			TX_buffer[2] = NODE_ID;
			TX_buffer[3] = cur_lux & 0xFF;
			TX_buffer[4] = (cur_lux >> 8) & 0xFF;
			TX_buffer[5] = cur_temp & 0xFF;
			TX_buffer[6] = (cur_temp >> 8) & 0xFF;
			TX_buffer[7] = prior_lux & 0xFF;
			TX_buffer[8] = ((prior_lux) >> 8) & 0xFF;
			TX_buffer[9] = prior_temp & 0xFF;
			TX_buffer[10] = ((prior_temp) >> 8) & 0xFF;

			sendMessage(TX_buffer, &dataLength);
		}

		// End of collection Interval
		timer_B0_interrupt_flag = LOW;


	} // End While


