### purpose
Software to run on the forgetmenot board based on ATtiny3217 (alternative ATtiny1617).
It measures temperature, humidity of soil and outputs result on led's, on a ST7789 screen or
sends the data via RFM95 or RMF69 to a server.
This is a complete new version of a [sensor based on the ATtiny88](https://github.com/2ni/attiny88).

It's more or a less a boilerplate at the moment. I'm working on the hardware.

### open points
- [ ] create hardware board
- [ ] write according software

### useful links
- no support megatinycores on pio yet: https://github.com/platformio/platform-atmelavr/issues/83#issuecomment-528268480

### programmer
- with simple ttl serial port: https://github.com/mraardvark/pyupdi
- https://github.com/SpenceKonde/megaTinyCore

### todos
- measure vcc w/o gpio: http://ww1.microchip.com/downloads/en/Appnotes/00002447A.pdf
- use cs on st7789 w/o CS: https://github.com/Bodmer/TFT_eSPI/issues/163#issuecomment-515616375
- decoupling caps: https://electronics.stackexchange.com/questions/90971/does-my-circuit-need-decoupling-caps
- use PTC (capacitive touch control) see http://ww1.microchip.com/downloads/en/appnotes/atmel-42360-ptc-robustness-design-guide_applicationnote_at09363.pdf


### install avr-gcc for avrxmega3 (ie attiny3217)
I tried many things including patching etc. At the end the following worked for me:
- use the [toolchain](https://www.microchip.com/mplab/avr-support/avr-and-arm-toolchains-c-compilers) from microchip with a [patch](https://www.avrfreaks.net/comment/2838941#comment-2838941)
- use [packs](http://packs.download.atmel.com/) from atmel (search for attiny3217 to get the correct pack)

1. download the newest toolchain from [microchip](https://www.microchip.com/mplab/avr-support/avr-and-arm-toolchains-c-compilers)
```
mkdir toolchain
tar xfz avr8-gnu-toolchain-osx-3.6.2.503-darwin.any.x86_64.tar.gz -C toolchain/ 
make patchtoolchain
```
2. download the [pack](http://packs.download.atmel.com/) for your device 
```
mkdir pack
unzip Atmel.ATtiny_DFP.1.4.283.atpack -d pack/
```


### old links trying to get things working
- https://www.avrfreaks.net/forum/solved-compiling-attiny1607-or-other-0-series1-series-avr-gcc
- trial 1
  - http://packs.download.atmel.com/ (search for attiny3217)
  - http://leonerds-code.blogspot.com/2019/06/building-for-new-attiny-1-series-chips.html
  - mkdir pack; cd pack; unzip ../Atmel.ATtiny_DFP.1.4.283.atpack
- trial 2:
  - The compile command should end up looking like:
    avr-gcc -g -Wall -Os -I /Downloads/Atmel.ATtiny_DFP.1.3.229/include/ -B /Downloads/Atmel.ATtiny_DFP.1.3.229/gcc/dev/attiny1607 -mmcu=attiny1607 -o foo.elf foo.c
    Where the  -B and -I options point into the uncompressed "packs" directory (it's a zip file, despite the weird extension), and are both necessary.
- trial 3 (toolchain from microchip):
  - https://www.microchip.com/mplab/avr-support/avr-and-arm-toolchains-c-compilers
- trial 4:
  - https://github.com/stevenj/avr-libc3
- trial 5:
  - https://bugs.launchpad.net/ubuntu/+source/gcc-avr/+bug/1754624
- trial 6:
  - https://bitwise.bperki.com/2019/02/18/getting-started-with-attiny104-part-1-installing-the-toolchain-on-macos/
- trial 7:
  - https://github.com/vladbelous/tinyAVR_gcc_setup 
  - https://github.com/richcarni/attiny402
- trial 8: (compile all from scratch)
  - https://www.avrfreaks.net/forum/building-gcc83-avr-libc-linux-w-atpack-hooks
