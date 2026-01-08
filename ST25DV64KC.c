/***************************
 * Author: Caden Allen
 * Date: TODO: 
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

#include "ST25DV64KC.h"



/*******************************
 *
 * GLOBAL VARIABLES
 *
 ***********************/ 

// Data to send with I2C interrupt
static volatile uint8_t * TXPayload;

// Length of data to send via I2C
static volatile uint8_t * TXLength;

// Keep track of bytes sent via I2C for current payload 
static volatile uint8_t TXByteCounter;

// Track if security session is open, necessary to write to user memory of the tag
static volatile uint8_t IsSecuritySessionOpen = 0;

// Define how many times to retry before failing
const int MAX_RETRIES = 3;

// Global Status of I2C 
static volatile I2C_Status Status;


// Length of the SecurityMessage to transmit 
static uint8_t START_SECURITY_SESSION_MESSAGE_LENGTH = 19;

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
static uint8_t START_SECURITY_SESSION_MESSAGE [19] = {
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
* Helper Function Implementations
*
**********************/ 


void sleepAndWriteI2C() {

    // Wait until stop condition has been sent
    while(UCB2CTLW0 & UCTXSTP);

    //Clear pending interrupt flags
    UCB2IFG = 0;

    //Become transmitter and send start condition
    UCB2CTLW0 |= UCTR | UCTXSTT;

    //Sleep and wait for data to be sent
    __bis_SR_register(LPM3_bits | GIE);
}







/***********************
 *
 * Static Function Implementations
 *
 **********************/ 



static I2C_Status openSecuritySession() {
    for(uint8_t x = 0; x < MAX_RETRIES; x++) {
     
        TXPayload = START_SECURITY_SESSION_MESSAGE;
        TXLength = &START_SECURITY_SESSION_MESSAGE_LENGTH;
        UCB2I2CSA = RFID_TAG_SECURITY_SESSION_CMD;

        sleepAndWriteI2C();

        if(Status == I2C_TRANSACTION_SUCCESS) {
            IsSecuritySessionOpen = 1;
            return Status;
        }
    }

    return I2C_TRANSACTION_NACK;
}





/***********************
 *
 * User Function Implementations
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
	UCB2CTLW0 |= UCMODE_3 | UCMST | UCSSEL__SMCLK;
	
	//Baudrate = SMCLK / 8
	//Possible to change which clock is selected for divison
	UCB2BRW = 0x0008;

    //Clear software reset, put I2C bus into operation
	UCB2CTLW0 &= ~UCSWRST;

	//Enable transmit and not ack interrupt
	UCB2IE |= UCTXIE0 | UCNACKIE;
}


void writeI2CByte() {
    Status = I2C_TRANSACTION_INPROGRESS;
     
	if(TXByteCounter < *TXLength){	
		
		//Load data into transmit buffer
		UCB2TXBUF = TXPayload[TXByteCounter];
		TXByteCounter++;
	
	} else {
		
		//Message is sent
		Status = I2C_TRANSACTION_SUCCESS;	

		//Resets byte counter
		TXByteCounter = 0; 	
	
	    UCB2CTLW0 |= UCTXSTP;
    }
}

//Sends a message given a array of bytes and length to I2C, returns if message was sent successfully or not
I2C_Status sendMessage(uint8_t * message, uint8_t messageLength) {
	 

    // Wait until stop condition has been sent
	while(UCB2CTLW0 & UCTXSTP);

	// Initial Status of NONE
	Status = I2C_TRANSACTION_NONE;
    
    if(!IsSecuritySessionOpen){
        openSecuritySession();      
    }
    
    UCB2I2CSA = RFID_TAG_USER_MEMORY_CMD;

	for (int x = 0; x < MAX_RETRIES; x++) {   
        TXPayload = message; 
        TXLength = &messageLength; 

		sleepAndWriteI2C();

       	//Check if message was sent successfully 
        if (Status == I2C_TRANSACTION_SUCCESS) {
			
            //Transacation was successful, return
            return I2C_TRANSACTION_SUCCESS;
		}
	}

	return I2C_TRANSACTION_NACK;
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
			UCB2IFG &= ~UCNACKIFG;
            
            TXByteCounter = 0;
			Status = I2C_TRANSACTION_NACK;

			//Break out of LPM3
			__bic_SR_register_on_exit(LPM3_bits);
			
			break;

		
		//Transmit interrupt
		case USCI_I2C_UCTXIFG0:
			
            writeI2CByte();

			// Check if the message was sent or not, use INPROGRESS because Status
			// can be NACK or SUCCESS
			if(Status != I2C_TRANSACTION_INPROGRESS){
				
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
