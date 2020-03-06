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
CLK        = 20000000
PRG        = usbtiny

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
AVRDUDE    = avrdude -c $(PRG) -p $(DEVICE) -b 19200 -P usb
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

# check programmer connectivity
test:
	$(AVRDUDE) -v

# flash program to mcu
flash: all
	$(AVRDUDE) -U flash:w:$(PRJ).hex:i

# write fuses to mcu
fuse:
	$(AVRDUDE) -U lfuse:w:$(LFU):m -U hfuse:w:$(HFU):m -U efuse:w:$(EFU):m

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
