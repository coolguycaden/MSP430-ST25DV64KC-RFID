#ifndef ST25DV64KC_H
#define ST25DV64KC_H

#include <msp430fr5994.h>
#include <stdint.h>
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

//Command code for presenting I2C password
// 0x57
#define RFID_TAG_SECURITY_SESSION_CMD 0b1010111


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
	I2C_TRANSACTION_BEGIN,
	I2C_TRANSACTION_INPROGRESS,
    I2C_TRANSACTION_SUCCESS,
    I2C_TRANSACTION_NACK,
    I2C_TRANSACTION_FINISHED
} I2C_Status;

typedef enum {
    TAG_SUCCESSFUL_WRITE,
    TAG_SUCCESSFUL_READ,
} Tag_Status; 


void setMaxRetries(uint8_t newRetries);
uint8_t getMaxRetries();

//Set config for I2C for msp430FR5994, setting the peripheral address with parameter
void initializeI2C();

//Send a message given an array and its length
//Set prependAddressFlag if you wish to have write address automatically calculated and injected
I2C_Status sendMessage(uint8_t * message, uint8_t * messageLength, uint8_t prependAddressFlag);
#endif 
