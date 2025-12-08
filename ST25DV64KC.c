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

/*
//Test dta to be transmitted
const unsigned char TXDataPayload [] = {
					RFID_TAG_STARTING_WRITE_ADDRESS_MSB,
					RFID_TAG_STARTING_WRITE_ADDRESS_LSB,
					0xD1, 0x22, 0x33, 0x44, 
					0x55, 0x66, 0x77, 0x88, 
					0x99, 0xAA, 0xBB, 0xCC,
					0xDD, 0xEE, 0xFF, 0xAB
				       }; 
					
//Length of TXDataPayload (that we want to send) 
const unsigned char TXDataLength = 18;
*/

volatile unsigned char * TXDataPayload;

volatile unsigned char * TXDataLength;


//boolean to determine message finished transmitting
static volatile unsigned char messageSent = 0;

//boolean to track if security session has been opened
static volatile unsigned char isSecuritySessionOpen = 0;


/* STATIC FUNC */


//Sets new peripheral address for I2C bus, necessary to switch commands
//for the RFID tag
void setNewPeripheralAddress(unsigned char peripheralAddress){

	//Set new address
	UCB2I2CSA = peripheralAddress;

}

void setNewPayload(volatile unsigned char * payload, volatile unsigned char * payloadLength){
	TXDataPayload = payload;
	TXDataLength = payloadLength;
}














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
	
	//Power CS
	P3OUT |= BIT7;
	P3DIR |= BIT7;


	//Configure I2C pins
	P7SEL0 |= BIT1 | BIT0;
	P7SEL1 &= ~(BIT1 | BIT0);
	
	P1SEL0 |= BIT6 | BIT7;
	P1SEL1 &= ~(BIT6 | BIT7);
	

	//Configure pull-up resistors for I2C bus (10k)
	P7REN |= BIT0 | BIT1; // pull up/down enabled
   	P7OUT |= BIT0 | BIT1; // pull up enabled

	P1REN |= BIT6 | BIT7; // pull up/down enabled
   	P1OUT |= BIT6 | BIT7; // pull up enabled


	//Software reset enabled, necessary to change configurations
	//I2C bus usable while this is seti
	UCB2CTLW0 |= UCSWRST; 

	//Enables I2C Mode, Controller mode, sync mode, select SMCLK
	UCB2CTLW0 |= UCMODE_3 | UCMST | UCSYNC | UCSSEL__SMCLK;

	//Baudrate = SMCLK / 8
	//Possible to change which clock is selected for divison
	UCB2BRW = 0x0008;

	//Peripheral Address
	UCB2I2CSA = RFID_TAG_I2C_SECURITY_SESSION_CMD;

	//Clear software reset, put I2C bus into operation
	UCB2CTLW0 &= ~UCSWRST;

	//Enable transmit and not ack interrupt
	UCB2IE |= UCTXIE0 | UCNACKIE; 
}




//Implements modular message sending, meant to be called multiple times in a row while
//TX interrupt is active for I2C. 
//This utilizes the hardware clock management of the MSP430 to make deadlines for data payload and ACKs
//Function should be called once per byte of a message (i.e. 11 byte message needs this to be called 11 times)
void writeI2CMessage(const volatile unsigned char * transmitData, const volatile unsigned char * dataLength){
	
	//Keep track of current byte to send
	static unsigned char TXByteCounter = 0;	
	
	//Keeps track of current address
	static unsigned int currentAddress = RFID_TAG_STARTING_WRITE_ADDRESS_FULL;


	if(TXByteCounter < *dataLength){	
		//Load data into transmit buffer
		UCB2TXBUF = transmitData[TXByteCounter];
		
		//increase current byte
		TXByteCounter++;
	
	} else {
		
		//Message is sent
		messageSent = 1;

		//Increase the currentAddress, setting it to next free space
		currentAddress += *dataLength;
		
		//If address greater than last writable address, wrap back to start
		//of writing space
		if(currentAddress >= RFID_TAG_ENDING_WRITE_ADDRESS_FULL){
			currentAddress = RFID_TAG_STARTING_WRITE_ADDRESS_FULL;

		}

		//Resets byte counter
		TXByteCounter = 0; 
		
		//Send stop condition
		UCB2CTLW0 |= UCTXSTP;

		//Clear transmit interrupt flag
		UCB2IFG &= ~UCTXIFG;
	
	}
}

//Opens the RFID Tag's security session, presents the default password with it
void startSecuritySession(){

	//Length of the message to transmit 
	const unsigned char MessageLength = 19;

	//Message to open security session of RFID Tag
	//RFID_TAG_I2C_PASSWORD repeated 8 times for the 64 bit password
	//Password MUST be sent TWICE to present password
	//the second "RFID_TAG_I2C_PASSWORD_ADDRESS_MSB" is a
	//verification step
	const unsigned char StartSecuritySessionMessage [] = {
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

	
	//Present password for RFID TAG	
	writeI2CMessage(StartSecuritySessionMessage, &MessageLength);
}

void sendMessage(volatile unsigned char * message, volatile unsigned char * messageLength){
	if(data[1] < RFID_TAG_ENDING_WRITE_ADDRESS_LSB){
		data[1] += dataLen;
	} else {
		data[1] = RFID_TAG_STARTING_WRITE_ADDRESS_LSB;
	}
	//Ensure stop condition is sent
	while(UCB2CTLW0 & UCTXSTP); 

	setNewPayload(message, messageLength);

	//Set I2C to TX, send start condition
	UCB2CTLW0 |= UCTR | UCTXSTT;

	//Enter low power mode and await interrupts
	__bis_SR_register(LPM3_bits | GIE);

	//If security session is open, writing is now open for user memory
	if(isSecuritySessionOpen){
		
		//Change target area to user memory to write to
		setNewPeripheralAddress(RFID_TAG_USER_MEMORY_CMD);
	} else {
		startSecuritySession();
	}

	//Prepare to send another message
	messageSent = 0;
}

int main(void) {

	//Disable watchdog
	WDTCTL = WDTPW | WDTHOLD;

	// Disable the GPIO power-on default high-impedance mode to activate
	// previously configured port settings
	PM5CTL0 &= ~LOCKLPM5;

	//Configure USCI_B2 for I2C Mode, 0 passed in for NO automatic stop
	initializeI2C(); 
	
	
	volatile unsigned char data [] = {
					  RFID_TAG_STARTING_WRITE_ADDRESS_MSB,
					  RFID_TAG_STARTING_WRITE_ADDRESS_LSB,
					  0xD0, 0xD1, 0xB2, 0xB3
					 };
	
	volatile unsigned char dataLen = 6;

	while(1){

		__delay_cycles(10000);
		sendMessage(data, &dataLen);
		
		
	}
}


// initializeI2C();
// sendMessage();

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
			
			//Break out of LPM3
			__bic_SR_register_on_exit(LPM3_bits);
			
			break;

		
		//Transmit interrupt
		case USCI_I2C_UCTXIFG0:
			
			//If security session NOT open, then open it 
			if(!isSecuritySessionOpen){
				startSecuritySession();

				//Checks if security message has been sent
				isSecuritySessionOpen = messageSent;
			} else {
				//Security session is open, able to write to memory	
				//Write payload
				writeI2CMessage(TXDataPayload, TXDataLength);
			}
			

			//Self-explanatory
			if(messageSent){

				//exit low power mode 3 	
				__bic_SR_register_on_exit(LPM3_bits);
			}
			
			break;
		
			
		default:
			break;
	}
}
