# inspired by 
# - https://gist.github.com/holachek/3304890
# - https://gist.github.com/mcous/5920089
#
# .elf needed on host for debugging
# .hex is code for programming to flash
#
# export PATH=/www/forgetmenot/toolchain/bin:$PATH
#
# run "make FLAGS=" to not run in debug mode

PRJ        = main
DEVICE     = attiny3217
DEVICE_PY  = $(shell echo $(DEVICE) | sed -e 's/^.*\(tiny\d*\)/\1/')
# default prescaler is 6 -> 3.3MHz
# max frequency is 13.3MHz for 3.3v (see p.554)
# set to 10MHz with cmd: _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc);
CLK        = 10000000
# PRG        = usbtiny

# TBD FUSES
# see http://www.engbedded.com/fusecalc/
LFU        = 0xAE
HFU        = 0xDF
EFU        = 0xFF

SRC        = ./src
EXT        =

LIB        = ./toolchain/avr/include/
BIN        = ./toolchain/bin/

# ********************************************************

INCLUDE   := $(foreach dir, $(EXT), -I$(dir))

FLAGS      = -DDEBUG
CFLAGS     = -Wall -Os -DF_CPU=$(CLK) -mmcu=$(DEVICE) -B pack/gcc/dev/$(DEVICE)/ -I pack/include/ -I $(LIB) $(FLAGS)
CPPFLAGS   =

# executables
#
# !!!!!!!!!!! always 1st connect programmer cable !!!!!!!!!! 
#
PORT_PRG   = $(shell ls -U /dev/cu.wchusbserial*|tail -1)
PORT_DBG   = $(shell if [ `ls -U /dev/cu.wchusbserial*|wc -l` -gt 1 ]; then ls -U /dev/cu.wchusbserial*|head -1; fi)
PYUPDI     = pyupdi.py -d $(DEVICE_PY) -c $(PORT_PRG)
# AVRDUDE    = avrdude -c $(PRG) -p $(DEVICE) -b 19200 -P usb
OBJCOPY    = $(BIN)avr-objcopy
OBJDUMP    = $(BIN)avr-objdump
SIZE       = $(BIN)avr-objdump -Pmem-usage
CC         = $(BIN)avr-gcc

# objects
CFILES     = $(wildcard $(SRC)/*.c)
EXTC      := $(foreach dir, $(EXT), $(wildcard $(dir)/*.c))
CPPFILES   = $(wildcard $(SRC)/*.cpp)
EXTCPP    := $(foreach dir, $(EXT), $(wildcard $(dir)/*.cpp))
OBJ        = $(CFILES:.c=.o) $(EXTC:.c=.o) $(CPPFILES:.cpp=.o) $(EXTCPP:.cpp=.o)
HEADERS   := $(foreach dir, $(SRC) $(EXT), $(wildcard $(dir)/*.h))


# user target
# compile all files
all: 	Makefile.done $(PRJ).hex

serial:
	@if [ -z $(PORT_DBG) ]; then echo "no DBG port found"; else ./serialterminal.py -p $(PORT_DBG) -d; fi

ports:
	@echo "available ports:"
	@unset CLICOLOR &&  ls -1t /dev/cu.wchusbserial* && export CLICOLOR=1
	@echo "configuration"
	@echo "PRG: $(PORT_PRG)"
	@if [ ! -z $(PORT_DBG) ]; then echo "DBG: $(PORT_DBG)"; fi


# check programmer connectivity
test:
	$(PYUPDI) -iv

# read out fuses
readfuse:
	$(PYUPDI) -fr

# write fuses to mcu
# TODO (this is just an example)
writefuse:
	$(PYUPDI) -fs 0:0x00 1:0x00

# flash program to mcu
flash: all
	@echo "flashing to $(PORT_PRG). Pls wait..."
	@$(PYUPDI) -f $(PRJ).hex && afplay /System/Library/Sounds/Ping.aiff -v 30

reset:
	@$(PYUPDI) -r && afplay /System/Library/Sounds/Ping.aiff -v 30

# generate disassembly files for debugging
disasm: $(PRJ).elf
	$(OBJDUMP) -d $(PROJ).elf

# cleanup
clean:
	rm -rf *.hex *.elf *.o
	$(foreach dir, $(EXT) $(SRC), rm -f $(dir)/*.o;)

# objects from c files
.c.o: $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# objects from c++ files
.cpp.o: $(HEADERS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# elf
$(PRJ).elf: $(OBJ)
	$(CC) $(CFLAGS) -o $(PRJ).elf $(OBJ)

# hex
$(PRJ).hex: $(PRJ).elf
	rm -f $(PRJ).hex
	$(OBJCOPY) -j .text -j .data -O ihex $(PRJ).elf $(PRJ).hex
	@echo ""
	@echo "***********************************************************"
	@$(SIZE) $< | egrep "Program|Data|Device"
	@echo "***********************************************************"

# make clean if makefile altered
Makefile.done: Makefile
	make clean
	@touch $@

#show_fuses:
#	@$(AVRDUDE) 2>&1 | grep Fuses
#
#toolchainpatch:
#	sed -i.bak 's/0x802000/__DATA_REGION_ORIGIN__/' $(find toolchain/ -type f -name 'avrxmega3*')
#

# # show available terminals
# # connect debug pin to ttl
# terminals:
# 	@./handle_serial.py --list --port=$$port

# # run terminal, ctrl+c to stop
# terminal:
# 	@./handle_serial.py --monitor --port=$$port

# # terminal with timestamps
# terminalt:
# 	@./handle_serial.py --monitor --port=$$port | ./ts

# ft: flash terminal
