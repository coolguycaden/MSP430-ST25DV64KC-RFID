DRIVER:= ezfet # mspdebug driver
DEVICE = msp430fr5994

ifndef MSPGCCDIR
	MSPGCCDIR=$(HOME)/ti/msp430-gcc
endif

UPDEVICE=$(shell echo $(DEVICE) | tr a-z A-Z)

#paths
SUPPORT_FILE_DIRECTORY = $(MSPGCCDIR)/include

# compiler options
CC      = $(MSPGCCDIR)/bin/msp430-elf-g++

CXXFLAGS = -I . -I $(SUPPORT_FILE_DIRECTORY) -mmcu=$(DEVICE) -g -Os -mhwmult=f5series -std=c++11
LFLAGS = -L . -L $(SUPPORT_FILE_DIRECTORY)


#### Compiling ####

INCL_SRC = $(wildcard includes/*.c)
INCL_H = $(wildcard includes/*.h)

all: ST25DV64KC.elf install_ST25DV64KC

# "$@ task label and $? is dependency list
ST25DV64KC.elf:	ST25DV64KC.c $(INCL_H) $(INCL_SRC)
	$(CC) $(CXXFLAGS) $(LFLAGS) -D __$(UPDEVICE)__ $? -o $@

# Upload to board
install_ST25DV64KC:	ST25DV64KC.elf
	mspdebug $(DRIVER) "prog ST25DV64KC.elf" --allow-fw-update

gdb:
	msp430-elf-gdb --ex "file ST25DV64KC.elf" --ex "target remote localhost:2000" --ex "load ST25DV64KC.elf"

clean:
	rm -f  *.o *.elf
