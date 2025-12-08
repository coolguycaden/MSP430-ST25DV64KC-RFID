#ifndef ST25DV64KC_H
#define ST25DV64KC_H


/***********************************
 *
 * ST25DV64KC Defined Commands
 *
 ***********************************/

// The ST25DV64KC operates via addressing, the first address sent signifying a command
//
// A command is set up like this:
//
// DEVICE SELECT - E2 E1 E0 
//
// For each command, the device select code will be the same, being the tag peripheral address
// Last four bits - 0b1010 - is the factory address for the tag
//
// E0 is set via registers, but by default is 1
//
// E2 and E1 define what memory to access, not important to list here


/***********************************
 *
 * RFID TAG COMMAND DEFINES
 *
 * - some commands are the same with different names,
 *   just easier to distinguish why to use them
 *
 ***********************************/ 


//User memory access command
// 0x53
#define RFID_TAG_USER_MEMORY_CMD 0b1010011 

//Static mailbox config access command
#define RFID_TAG_STATIC_MAILBOX_CMD 0b1010111

//Dynamic mailbox access command
#define RFID_TAG_DYN_MAILBOX_CMD 0b1010011

//Command code for presenting I2C password
// 0x57
#define RFID_TAG_SECURITY_SESSION_CMD 0b1010111


/*********************************
 *
 *
 * RFID TAG MAILBOX ADDRESS DEFINES
 *
 * Bits are numbered right to left 
 ********************************/ 

//Address for start of Fast Transfer Mailbox
//most significant byte of address (bits 8-15)
#define RFID_TAG_MAILBOX_START_ADDRESS_MSB 0x20

//least significant byte of address (bits 0-7)
#define RFID_TAG_MAILBOX_START_ADDRESS_LSB 0x08



//Address for end of Fast Transfer Mailbox
//most significant byte of address (bits 8-15)
#define RFID_TAG_MAILBOX_END_ADDRESS_MSB 0x21

//least significant byte of address (bits 0-7)
#define RFID_TAG_MAILBOX_END_ADDRESS_LSB 0x07


/*************************************************
 *
 * RFID TAG WRITING MEMORY AREA DEFINES
 *
 *************************************************/


//Address for the area we will start writing at
//Most significant byte (bits 8-15)
#define RFID_TAG_STARTING_WRITE_ADDRESS_MSB 0x00

//Least signifcant byte (bits 7-0)
#define RFID_TAG_STARTING_WRITE_ADDRESS_LSB 0x24

//Combined for reference
#define RFID_TAG_STARTING_WRITE_ADDRESS_FULL 0x0024


//Address for area where we will stop writing at
//Most signifcant byte (bits 8-15)
#define RFID_TAG_ENDING_WRITE_ADDRESS_MSB 0x00

//least signifcant byte (bits 7-0)
#define RFID_TAG_ENDING_WRITE_ADDRESS_LSB 0xFF

//Combined for reference
#define RFID_TAG_ENDING_WRITE_ADDRESS_FULL 0x00FF

/*************************************************
 *
 * RFID TAG REGISTER DEFINES
 *
 *************************************************/ 

//Static register for mailbox config of the
//RFID tag 
#define RFID_TAG_MAILBOX_STATIC_REGISTER_ADDRESS_MSB 0x00
#define RFID_TAG_MAILBOX_STATIC_REGISTER_ADDRESS_LSB 0x0D


//Dynamic register for mailbox config of the
//RFID tag
#define RFID_TAG_MAILBOX_DYN_REGISTER_ADDRESS_MSB 0x20
#define RFID_TAG_MAILBOX_DYN_REGISTER_ADDRESS_LSB 0x06

//Bit to enable mailbox is the lowest order bit
#define RFID_TAG_MAILBOX_MODE_ENABLE_BIT 0b00000001



/*******************************
 *
 * I2C PASSWORD COMMAND DEFINES
 *
 *******************************/



//The most significant byte of the password address
//bits (8-15)
#define RFID_TAG_I2C_PASSWORD_ADDRESS_MSB 0x09 

//The least significant byte of the password address
#define RFID_TAG_I2C_PASSWORD_ADDRESS_LSB 0x00


//The password for the I2C security session
//The password is 64 bits long, and by default just eight 0x00 s
#define RFID_TAG_I2C_PASSWORD 0x00

/***************************************
 *
 * End defines
 *
 ***************************************/



/***************************************
 *
 * Optional Variables
 * - make life easier to have them
 ***************************************/


// Plain and easy way to understand and set if 
// a given I2C transaction successful?
typedef enum {
    I2C_TRANSACTION_NONE, 
    I2C_TRANSACTION_SUCCESS,
    I2C_TRANSACTION_NACK,
	I2C_TRANSACTION_INPROGRESS,
	I2C_TRANSACTION_BEGIN
} I2C_Status;


//Set config for I2C for msp430FR5994, setting the peripheral address with parameter
void initializeI2C();

//Send a message given an array and its length
//I2C_Status sendMessage(volatile unsigned char * message, volatile unsigned char messageLength);
//I2C_Status sendMessage();
#endif 
