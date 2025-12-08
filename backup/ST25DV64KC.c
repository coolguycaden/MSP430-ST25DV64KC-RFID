/***************************
 * Author: Caden Allen
 * Date: 2/21/2025
 * 
 * Purpose: Simple RFID communication between msp430FR5994 and
 * ST25DV64KC. To be implemented into Dr. Tobias' project.
 *
 *
 *
 *
 *
 *
 ***************************/ 

#include <msp430fr5994.h>
#include "ST25DV64KC.h"



/*******************************
 *
 * GLOBAL VARIABLES
 *
 ***********************/ 

//Data to be sent
static volatile unsigned char * data = 0;

//Length of data to be sent in bytes
static volatile unsigned char dataLen = 0;

//Keep track of current byte to send
static volatile unsigned char TXByteCounter = 0;	

//Track if security session is open, necessary to write to user memory of the tag
static volatile unsigned char isSecuritySessionOpen = 0;

//Define how many times to retry before failing
const int MAX_RETRIES = 3;
    
static volatile I2C_Status status;


//Length of the SecuriryMessage to transmit 
static volatile unsigned char SecurityMessageLength = 19;
#define length 19

// Message to open security session of RFID Tag.
//
// RFID_TAG_I2C_PASSWORD repeated 8 times for the 64 bit password.
//
// Password MUST be sent TWICE to present password.
//
// The second "RFID_TAG_I2C_PASSWORD_ADDRESS_MSB" is a
// verification step.
//
// In this case, the PASSWORD define is just 0s (no password)
static volatile unsigned char StartSecuritySessionMessage [19] = {
	RFID_TAG_I2C_PASSWORD_ADDRESS_MSB,
	RFID_TAG_I2C_PASSWORD_ADDRESS_LSB,
	RFID_TAG_I2C_PASSWORD, 
	RFID_TAG_I2C_PASSWORD, 
	RFID_TAG_I2C_PASSWORD, 
	RFID_TAG_I2C_PASSWORD, 
	RFID_TAG_I2C_PASSWORD, 
	RFID_TAG_I2C_PASSWORD, 
	RFID_TAG_I2C_PASSWORD, 
	RFID_TAG_I2C_PASSWORD, 
	RFID_TAG_I2C_PASSWORD_ADDRESS_MSB,
	RFID_TAG_I2C_PASSWORD, 
	RFID_TAG_I2C_PASSWORD, 
	RFID_TAG_I2C_PASSWORD, 
	RFID_TAG_I2C_PASSWORD, 
	RFID_TAG_I2C_PASSWORD, 
	RFID_TAG_I2C_PASSWORD, 
	RFID_TAG_I2C_PASSWORD, 
	RFID_TAG_I2C_PASSWORD
};	







/***********************
 *
 * Function Implementations
 *
 **********************/ 


/******************************************
 * Initialize USCI_B2 for I2C
 *
 * Acronyms are explained line by line
 *
 * UCB2CTLW0/1 -> eUSCI_B2 Control Word 0/1
 * UCB2TBCNT -> Byte Counter Threshold
 * UCB2I2CSA -> I2C Peripheral Address
 */ 
void initializeI2C(){
	
	//Configure I2C pins
	P7SEL0 |= BIT1 | BIT0;
	P7SEL1 &= ~(BIT1 | BIT0);
	
	//Configure pull-up resistors for I2C bus (10k)
	P7REN |= BIT0 | BIT1; // pull up/down enabled
   	P7OUT |= BIT0 | BIT1; // pull up enabled

	//Software reset enabled, necessary to change configurations
	//I2C bus usable while this is seti
	UCB2CTLW0 |= UCSWRST; 

	//Enables I2C Mode, Controller mode, sync mode, select SMCLK
	UCB2CTLW0 |= UCMODE_3 | UCMST | UCSYNC | UCSSEL__SMCLK;
	
	//Baudrate = SMCLK / 8
	//Possible to change which clock is selected for divison
	UCB2BRW = 0x0008;

    //Clear software reset, put I2C bus into operation
	UCB2CTLW0 &= ~UCSWRST;

	//Enable transmit and not ack interrupt
	UCB2IE |= UCTXIE0 | UCNACKIE;
}




//Implements modular message sending, meant to be called multiple times in a row while
//TX interrupt is active for I2C. 
//This utilizes the hardware clock management of the MSP430 to make deadlines for data payload and ACKs
//Function should be called once per byte of a message (i.e. 11 byte message needs this to be called 11 times)
/*void writeI2CMessage(const volatile unsigned char * transmitData, const volatile unsigned char * dataLength){
	
	status = I2C_TRANSACTION_INPROGRESS;

	if(TXByteCounter < *dataLength){	
		
		//Load data into transmit buffer
		UCB2TXBUF = transmitData[TXByteCounter];
		TXByteCounter++;
	
	} else {
		
		//Message is sent
		status = I2C_TRANSACTION_SUCCESS;	

		//Resets byte counter
		TXByteCounter = 0; 
	
	}
}*/
void writeI2CMessage() {
    status = I2C_TRANSACTION_INPROGRESS;
    

	if(TXByteCounter < dataLen){	
		
		//Load data into transmit buffer
		UCB2TXBUF = data[TXByteCounter];
		TXByteCounter++;
	
	} else {
		
		//Message is sent
		status = I2C_TRANSACTION_SUCCESS;	

		//Resets byte counter
		TXByteCounter = 0; 
	
	}
}


//Sends a message given a array of bytes and length to I2C, returns if message was sent successfully or not
I2C_Status sendMessage(volatile unsigned char * message, volatile unsigned char messageLength) {
	 

	// Wait until stop condition has been sent
	while(UCB2CTLW0 & UCTXSTP);

	// Initial status of NONE
	status = I2C_TRANSACTION_NONE;



	for (int x = 0; x < MAX_RETRIES; x++) {

        // If the first attempt fails (NACK'd),
		// assume that the security session was NOT open
		if (!isSecuritySessionOpen) {

			// Set peripheral address to open security session
			UCB2I2CSA = RFID_TAG_SECURITY_SESSION_CMD;

			data = StartSecuritySessionMessage;
			dataLen = SecurityMessageLength;

		} else {

			// Set peripheral address to write to user memory
			UCB2I2CSA = RFID_TAG_USER_MEMORY_CMD;


			// Attempt to send user's message
			// Set global variables for ISR
			data = message;
			dataLen = messageLength;	
		}

		// Wait until stop condition has been sent
		while(UCB2CTLW0 & UCTXSTP);

		//Clear pending interrupt flags
		UCB2IFG = 0;
		
		//Become transmitter and send start condition
		UCB2CTLW0 |= UCTR | UCTXSTT;
		
		//Sleep and wait for data to be sent
        __bis_SR_register(LPM3_bits | GIE);
        
       	//Check if message was sent successfully 
        if (status == I2C_TRANSACTION_SUCCESS) {
			isSecuritySessionOpen = status == I2C_TRANSACTION_SUCCESS ? 1 : 0;  

			//Transacation was successful, return
            return I2C_TRANSACTION_SUCCESS;
		}
	}

	return I2C_TRANSACTION_NACK;
}



int main(void) {

	//Disable watchdog
	WDTCTL = WDTPW | WDTHOLD;

	// Disable the GPIO power-on default high-impedance mode to activate
	// previously configured port settings
	PM5CTL0 &= ~LOCKLPM5;

	//Configure USCI_B2 for I2C Mode
	initializeI2C(); 
	
	volatile unsigned char dataLength = 16;
	/*volatile unsigned char TX_buffer[dataLength];
    
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
    TX_buffer[15] = 0x15;*/

    volatile unsigned char TX_buffer[dataLength] = {
        RFID_TAG_STARTING_WRITE_ADDRESS_MSB,
        RFID_TAG_STARTING_WRITE_ADDRESS_LSB,
        0xAA,
        0x03,
        0x04,
        0x05,
        0x06,
        0x07,
        0x08,
        0x09,
        0x10,
        0x11,
        0x12,
        0x13,
        0x14,
        0x15
    };


	
	__delay_cycles(3000000);
	
	int messages = 0;
	while(messages < 3){
		
		__delay_cycles(1000000);
		sendMessage(TX_buffer, dataLen);
		messages += 1;
	}

	return 0;
}




/*****************************************
 *
 *
 *
 *  I2C Interrupt Handling
 *
 *
 *
 *
 *****************************************/ 

#if defined(__TI_COMPILER_VERSION__) || defined (__IAR_SYSTEMS_ICC__)
#pragma vector = EUSCI_B2_VECTOR
__interrupt void USCI_B2_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(EUSCI_B2_VECTOR))) USCI_B2_ISR (void)
#else 
#error C/ompiler not supported!
#endif
{
	switch(__even_in_range(UCB2IV, USCI_I2C_UCBIT9IFG)){
	
		//No acknowledge flag
		case USCI_I2C_UCNACKIFG: 
		
			//Must send stop condition
			UCB2CTLW0 |= UCTXSTP;
			
			//Clear NACK interrupt flag
			UCB2IFG &= ~UCTXIFG;
            
            TXByteCounter = 0;
			status = I2C_TRANSACTION_NACK;

			//Break out of LPM3
			__bic_SR_register_on_exit(LPM3_bits);
			
			break;

		
		//Transmit interrupt
		case USCI_I2C_UCTXIFG0:
			
			//If security session NOT open, then open it 
			//Unable to write to tag unless session is open, so keep retrying each time
			/*if(!isSecuritySessionOpen){
				startSecuritySession();
				isSecuritySessionOpen = status == I2C_TRANSACTION_SUCCESS ? 1 : 0;  
			}*/

			// Check again, we still want to send original message if we opened security session
			// successfully
			//if(isSecuritySessionOpen){
					
				// Write user payload
			//writeI2CMessage(data, &dataLen);
			//}
			writeI2CMessage();

			// Check if the message was sent or not, use INPROGRESS because status
			// can be NACK or SUCCESS
			if(status != I2C_TRANSACTION_INPROGRESS){
				
				//Clear transmit interrupt flag
				UCB2IFG &= ~UCTXIFG;

				//exit low power mode 3 	
				__bic_SR_register_on_exit(LPM3_bits);
				
			}

			break;
		
			
		default:
			break;
	}
}
