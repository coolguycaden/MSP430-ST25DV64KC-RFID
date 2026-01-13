## MSP430FR5994 RFID with ST25DV64KC
This project implements an I2C interface between an MSP430FR5994 microcontroller and an ST25DV64KC Dynamic NFC/RFID tag. It writes data packets to the tag's user memory and utilizes onboard LEDs to indicate transaction status.

# Hardware Requirements
* Microcontroller: MSP-EXP430FR5994 LaunchPad Development Kit
* RFID Tag: ST25DV64KC (Dynamic NFC/RFID tag)
* Connections: Tag -> Launchpad
    * SDA -> P7.0
    * SCL -> P7.1
    * 3v3 -> 3v3 
    * GND -> Common Ground

## Linux Development Environment Setup
This project uses the official TI MSP430 GCC toolchain and `mspdebug` for flashing.

# 1. Install Dependencies
Install `make` and `mspdebug` via your package manager
```bash
sudo apt-get update
sudo apt-get install make mspdebug
```

# 2. Install TI MSP430 GCC
The Makefile is configured to look for the compiler in `~/ti/msp430-gcc`.
1. Download the MSP430-GCC-OPENSOURCE (Linux 64-bit) installer from [Texas Instruments](https://www.ti.com/tool/MSP430-GCC-OPENSOURCE)
2. Run the installer and set the installation directory to `~/ti/msp430-gcc`.
    * Alternatively, if you install it elsewhere, you must export the path:
    ```bash
    export MSPGCCDIR=/your/custom/path/msp430-gcc
    ```

## How to Build and Run
# Compiling
Just run the below to generate the `.elf`:
```bash
make
```
# Flashing to Hardware
To compile and immediately flash the program to the MSP430FR5994 LaunchPad:
```bash
make install
```
