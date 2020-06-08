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
.PHONY: all clean

PRJ        = main
DEVICE     = attiny3217
DEVICE_PY  = $(shell echo $(DEVICE) | sed -e 's/^.*\(tiny\d*\)/\1/')
# default prescaler is 6 -> 3.3MHz
# max frequency is 13.3MHz for 3.3v (see p.554)
# set to 10MHz with cmd: _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc);
CLK        = 10000000

# TBD FUSES
# see http://www.engbedded.com/fusecalc/
LFU        = 0xAE
HFU        = 0xDF
EFU        = 0xFF

SRC        = ./src
EXT        =

FLAGS      = -DDEBUG
CPPFLAGS   =

# toolchain_microchip
# BIN        = ./toolchain/bin/
# LIB        = ./toolchain_microchip/avr/include/
# CFLAGS     = -Wall -Os -DF_CPU=$(CLK) -mmcu=$(DEVICE) -B toolchain_microchip/pack/gcc/dev/$(DEVICE)/ -I toolchain_microchip/pack/include/ -I $(LIB) $(FLAGS)

# toolchain_markr42
LIB        = ./toolchain_markr42/avr/avr/include/
BIN        = ./toolchain_markr42/avr/bin/
CFLAGS     = -Wall -Os -DF_CPU=$(CLK) -mmcu=$(DEVICE) -I $(LIB) $(FLAGS)

# ********************************************************

# executables
#
# !!!!!!!!!!! always 1st connect programmer cable !!!!!!!!!! 
#
PORT_PRG   = $(shell ls -U /dev/cu.wchusbserial*|tail -1)
PORT_DBG   = $(shell if [ `ls -U /dev/cu.wchusbserial*|wc -l` -gt 1 ]; then ls -U /dev/cu.wchusbserial*|head -1; fi)
PYUPDI     = pyupdi.py -d $(DEVICE_PY) -c $(PORT_PRG)
OBJCOPY    = $(BIN)avr-objcopy
OBJDUMP    = $(BIN)avr-objdump
SIZE       = $(BIN)avr-objdump -Pmem-usage
CC         = $(BIN)avr-gcc

# objects
CFILES     = $(wildcard $(SRC)/*.c)
EXTC      := $(foreach dir, $(EXT), $(wildcard $(dir)/*.c))
CPPFILES   = $(wildcard $(SRC)/*.cpp)
EXTCPP    := $(foreach dir, $(EXT), $(wildcard $(dir)/*.cpp))
OBJ        = $(CFILES:.c=.c.o) $(EXTC:.c=.c.o) $(CPPFILES:.cpp=.cpp.o) $(EXTCPP:.cpp=.cpp.o)
DEPENDS   := $(CFILES:.c=.d) $(EXTC:.c=.d) $(CPPFILES:.cpp=.cpp.d) $(EXTCPP:.cpp=.cpp.d)

# user target
# compile all files
all: 	$(PRJ).hex

clean:
	rm -f *.hex *.elf
	rm -f $(OBJ) $(DEPENDS)

# objects from c files
%.c.o: %.c Makefile
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# objects from c++ files
%.cpp.o: %.cpp Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -MMD -MP -c $< -o $@

# linking, create elf
$(PRJ).elf: $(OBJ) Makefile
	$(CC) $(CFLAGS) -o $(PRJ).elf $(OBJ)

# create hex
$(PRJ).hex: $(PRJ).elf Makefile
	$(OBJCOPY) -j .text -j .data -j .rodata -O ihex $(PRJ).elf $(PRJ).hex
	@echo ""
	@echo "***********************************************************"
	@$(SIZE) $(PRJ).elf | egrep "Program|Data|Device"
	@echo "***********************************************************"

-include $(DEPENDS)

# start debugging terminal
serial:
	@if [ -z $(PORT_DBG) ]; then echo "no DBG port found"; else ./serialterminal.py -p $(PORT_DBG) -d; fi

# show available uart ports for flashing and debugging terminal
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

# flash hex to mcu
flash: all
	@echo "flashing to $(PORT_PRG). Pls wait..."
	@$(PYUPDI) -f $(PRJ).hex && afplay /System/Library/Sounds/Ping.aiff -v 30

# reset device
reset:
	@$(PYUPDI) -r && afplay /System/Library/Sounds/Ping.aiff -v 30

# generate disassembly files for debugging
disasm: $(PRJ).elf
	#$(OBJDUMP) -d $(PRJ).elf
	$(OBJDUMP) -S $(PRJ).elf > $(PRJ).asm
	$(OBJDUMP) -t $(PRJ).elf > $(PRJ).sym
	$(OBJDUMP) -t $(PRJ).elf |grep 00 |sort  > $(PRJ)-sorted.sym

patch_toolchain_microchip:
	sed -i.bak 's/0x802000/__DATA_REGION_ORIGIN__/' $(find toolchain_microchip/ -type f -name 'avrxmega3*')

patch_toolchain_markr42:
	sed -i.bak 's/PREFIX=[^ ]*/PREFIX=$$(pwd)\/avr/' toolchain_markr42/build.sh
