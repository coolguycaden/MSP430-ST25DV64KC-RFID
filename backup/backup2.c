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


//Test dta to be transmitted
const unsigned char TXDataPayload [] = {
					RFID_TAG_STARTING_WRITE_ADDRESS_MSB,
					RFID_TAG_STARTING_WRITE_ADDRESS_LSB,
					0x11, 0x22, 0x33, 0x44, 
					0x55, 0x66, 0x77, 0x88, 
					0x99, 0xAA, 0xBB, 0xCC,
					0xDD, 0xEE, 0xFF, 0xAB
				       }; 
					
//Length of TXDataPayload (that we want to send) 
const unsigned char TXDataLength = 18;

//boolean to determine message finished transmitting
volatile unsigned char messageSent = 0;

//boolean to track if security session has been opened
volatile unsigned char isSecuritySessionOpen = 0;

//Keeps track of current address
volatile unsigned int currentAddress = RFID_TAG_STARTING_WRITE_ADDRESS_FULL;

//keeps track of if automatic stop is set or not
volatile unsigned char automaticStopSet = 0;

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
void initializeI2C(unsigned char peripheralAddress, unsigned char automaticStop){
	
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
	
	//Sets default stop set as 0
	automaticStopSet = 0;

	//Determines if automatic stop is set
	if(automaticStop){
		
		//Automatic stop generated after UCB2TBCNT is reached
		UCB2CTLW1 |= UCASTP_2;
	
		//Number of bytes to be received
		//Needed for automatic stop generation
		UCB2TBCNT = automaticStop;
	
		automaticStopSet = 1;
	}

	//Baudrate = SMCLK / 8
	//Possible to change which clock is selected for divison
	UCB2BRW = 0x00020;

	//Peripheral Address
	UCB2I2CSA = peripheralAddress;

	//Clear software reset, put I2C bus into operation
	UCB2CTLW0 &= ~UCSWRST;

	//Enable transmit and not ack interrupt
	UCB2IE |= UCTXIE0 | UCNACKIE;
}




//Implements modular message sending, meant to be called multiple times in a row while
//TX interrupt is active for I2C. 
//This utilizes the hardware clock management of the MSP430 to make deadlines for data payload and ACKs
//Function should be called once per byte of a message (i.e. 11 byte message needs this to be called 11 times)
void writeI2CMessage(const unsigned char * transmitData, const unsigned char * dataLength){
	
	//Keep track of current byte to send
	static unsigned char TXByteCounter = 0;	
	
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
		
		//If automatic stop NOT set, then send stop condition manually	
		if(!automaticStopSet){
			//Send stop condition
			UCB2CTLW0 |= UCTXSTP;
		} else {
			//Stop condition stop automatically, reset byte counter?
			//Not sure if this is necessary
			UCB2TBCNT = TXDataLength;
		}

		//Clear transmit interrupt flag
		UCB2IFG &= ~UCTXIFG;
	
	}
}


//Sets new peripheral address for I2C bus, necessary to switch commands
//for the RFID tag
void setNewPeripheralAddress(unsigned char peripheralAddress){

	//Software reset enabled
	UCB2CTLW0 |= UCSWRST;

	//Set new address
	UCB2I2CSA = peripheralAddress;

	//Clear software reset
	UCB2CTLW0 &= ~UCSWRST;
}


//Send messages to enable 
void enableTagMailboxes(){
	
	//Message format to access static mailbox and enable it
	const unsigned char StaticMailboxEnableMessage [] = {
							     RFID_TAG_MAILBOX_STATIC_REGISTER_ADDRESS_MSB,
							     RFID_TAG_MAILBOX_STATIC_REGISTER_ADDRESS_LSB,
							     RFID_TAG_MAILBOX_MODE_ENABLE_BIT
							    };

	//Message format to access dynamic mailbox and enable it
	const unsigned char DynMailboxEnableMessage [] = {
							  RFID_TAG_MAILBOX_DYN_REGISTER_ADDRESS_MSB,
			       			          RFID_TAG_MAILBOX_DYN_REGISTER_ADDRESS_LSB,
						          RFID_TAG_MAILBOX_MODE_ENABLE_BIT
						         };
	
	//Length of messages
	const unsigned char MailboxMessageLength = 3;

	//Send enable message
	writeI2CMessage(StaticMailboxEnableMessage, &MailboxMessageLength);
	
	//Set peripheral address to write command for the dynamic mailbox
	setNewPeripheralAddress(RFID_TAG_DYN_MAILBOX_CMD);

	//Send next enable message
	writeI2CMessage(DynMailboxEnableMessage, &MailboxMessageLength); 

	//Set peripheral address to be write command for static mailbox
	setNewPeripheralAddress(RFID_TAG_STATIC_MAILBOX_CMD);
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

int main(void) {

	//Disable watchdog
	WDTCTL = WDTPW | WDTHOLD;

	// Disable the GPIO power-on default high-impedance mode to activate
	// previously configured port settings
	PM5CTL0 &= ~LOCKLPM5;

	//Configure USCI_B2 for I2C Mode, 0 passed in for NO automatic stop
	initializeI2C(RFID_TAG_I2C_SECURITY_SESSION_CMD, 0); 
	
	//Ensures first message is to present password,
	//and then sets messages for writing to user memory
	unsigned char isUserMemory = 0;
	
	//Give some time to start logic analyzer
	__delay_cycles(2000000);	
	
	while(1){

	
		//delay between messages
		__delay_cycles(200000);
		
		//Ensure stop condition is sent
		while(UCB2CTLW0 & UCTXSTP); 
		
		//Set I2C to TX, send start condition
		UCB2CTLW0 |= UCTR | UCTXSTT;

		//Enter low power mode and await interrupts
		__bis_SR_register(LPM3_bits | GIE);
	
		//After security session is opened, set new RFID cmd,
		//now targeting to write to user memory
		if(!isUserMemory){

			//Passes in the length of the message, enabling automatic stop
			initializeI2C(RFID_TAG_USER_MEMORY_CMD, 0);//TXDataLength);
			isUserMemory = 1;
		}	

		//Prepare to send another message
		messageSent = 0;
	}
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
	
		//No acknowledge flag
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
				writeI2CMessage(TXDataPayload, &TXDataLength);
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
