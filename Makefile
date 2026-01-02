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
TARGET = main

#### Compiling ####

INCL_SRC = $(wildcard *.c)
INCL_H = $(wildcard *.h)

all: $(TARGET) install clean

# "$@ task label and $? is dependency list
$(TARGET):	$(TARGET).c $(INCL_H) $(INCL_SRC) $(INCL_ST)
	$(CC) $(CXXFLAGS) $(LFLAGS) -D __$(UPDEVICE)__ $? -o $@.elf 

bear:
	bear -- $(TARGET)

# Upload to board
install:	$(TARGET)
	mspdebug $(DRIVER) "prog $(TARGET).elf" --allow-fw-update

gdb:
	msp430-elf-gdb --ex "file $(TARGET).elf" --ex "target remote localhost:2000" --ex "load $(TARGET).elf"

clean:
	rm -f  *.o *.elf
