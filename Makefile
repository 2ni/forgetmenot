# via https://gist.github.com/holachek/3304890
#
# .elf needed on host for debugging
# .hex is code for programming to flash
#
# export PATH=/www/forgetmenot/toolchain/bin:$PATH
#
# call make CFLAGS= to not run in debug mode

TOOLCHAIN  = ./toolchain/bin/
DEVICE     = attiny3217
CLOCK      = 20000000
# PROGRAMMER = -c usbtiny -P usb -b 19200
PROGRAMMER = -c usbtiny -P usb
OBJECTS    = src/main.o
# TBD
# FUSES      = -U lfuse:w:0xAE:m -U hfuse:w:0xDF:m -U efuse:w:0xFF:m
CFLAGS    = -DEBUG

AVRDUDE = avrdude $(PROGRAMMER) -p $(DEVICE) $(CFLAGS)
COMPILE = $(TOOLCHAIN)avr-gcc -Wall -B pack/gcc/dev/$(DEVICE)/ -I pack/include/ -Os -DF_CPU=$(CLOCK) -mmcu=$(DEVICE)
SIZE = $(TOOLCHAIN)avr-objdump -Pmem-usage

all: 	main.hex

# make clean if Makefile altered
# https://stackoverflow.com/questions/3871444/making-all-rules-depend-on-the-makefile-itself
-include dummy
dummy: Makefile
	@touch $@
	$(MAKE) -s clean

.c.o .cpp.o:
	$(COMPILE) -c $< -o $@

flash: 	all
	$(AVRDUDE) -U flash:w:main.hex:i

fuse:
	$(AVRDUDE) $(FUSES)

clean:
	rm -f main.hex main.elf $(OBJECTS)

check:
	@$(AVRDUDE) -v

show_fuses:
	@$(AVRDUDE) 2>&1 | grep Fuses

main.elf: $(OBJECTS)
	$(COMPILE) -o main.elf $(OBJECTS)

main.hex: main.elf
	rm -f main.hex
	$(TOOLCHAIN)avr-objcopy -j .text -j .data -O ihex main.elf main.hex
	@echo ""
	@echo "***********************************************************"
	@$(SIZE) $< | egrep "Program|Data|Device"
	@echo "***********************************************************"

toolchainpatch:
	sed -i.bak 's/0x802000/__DATA_REGION_ORIGIN__/' $(find toolchain/ -type f -name 'avrxmega3*')

# flash: compile
# 	@avrdude -p t88 -c usbtiny -U flash:w:.pio/build/attiny88/firmware.hex:i -F -P usb
# 	@#pio run -t program doesn't work anymore


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
