#include "ST25DV64KC.h"



/******************************************************************************
 * Globals
 ******************************************************************************/
// Size of I2C message
#define TX_BUFFER_SIZE 11        

// ID of each node
#define NODE_ID 1 

// ONE_SECOND is the amount of cycles at a set MHz.
// Base clock for MSP430FR5994 is 1 MHz, so 1,000,000 cycles in a second
// Change this to match the clock rate you set
#define ONE_SECOND 1000000




/******************************************************************************
 * Variable Declaration
 ******************************************************************************/

// Initialize TX buffer
uint8_t TX_buffer[TX_BUFFER_SIZE] = {0};     


void initializeLEDs() {
    P1DIR |= BIT0 | BIT1;
    P1OUT &= ~BIT0;
    P1OUT &= ~BIT1; 
}

void checkStatus(I2C_Status status) {
    if(status == I2C_TRANSACTION_NACK) {
        P1OUT ^= BIT0;
    } else if(status == I2C_TRANSACTION_SUCCESS) {
        P1OUT ^= BIT1;
    }

    __delay_cycles(ONE_SECOND); 
}




// Example Main Function
int main(void) {

	//Disable watchdog
	WDTCTL = WDTPW | WDTHOLD;

	// Disable the GPIO power-on default high-impedance mode to activate
	// previously configured port settings
	PM5CTL0 &= ~LOCKLPM5;

	//Configure USCI_B2 for I2C Mode
	initializeI2C();
	initializeLEDs();

    int messages = 0;

    while(1) {
        __delay_cycles(ONE_SECOND * 3);
        if(messages > 5) {
            // stop sending messages after 5 messages
            break;
        }
        
        // Set data to send
        TX_buffer[0] = RFID_TAG_STARTING_WRITE_ADDRESS_MSB;
        TX_buffer[1] = RFID_TAG_STARTING_WRITE_ADDRESS_LSB;
        TX_buffer[2] = NODE_ID;
        TX_buffer[3] = 0xAA;
        TX_buffer[4] = 0xBB; 
        TX_buffer[5] = 0xCC;
        TX_buffer[6] = 0xDD;
        TX_buffer[7] = 0xEE;
        TX_buffer[8] = 0xEF;
        TX_buffer[9] = 0xDD;
        TX_buffer[10] = 0x03; 
        
        I2C_Status status = sendMessage(TX_buffer, TX_BUFFER_SIZE);
        checkStatus(status);
        messages++;
    }
}
