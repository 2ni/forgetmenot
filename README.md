### purpose
This is the software used for my forgetmenot board based on ATtiny3217 (alternative ATtiny1617, ATtiny817).
See [hardware](hardware).
It measures temperature, humidity of soil and outputs result on led's, on a ST7789 screen or
sends the data via RFM95 or RMF69 to a server.
This is a complete new version of a [sensor based on the ATtiny88](https://github.com/2ni/attiny88).

### installation
```
pyenv virtualenv 3.7.8 forgetmenot
pyenv local forgetmenot
pip install -r requirements
./activate.py node
make build port=3
```

### examples
There are many examples in the ./examples folder. To upload one of it:
```
./activate.py examples/blink
make build port=3
```

### serial output
You can connect the usb of your computer to the usb and run
```
make serial port=1
```

### available ports
Depending on where you connect the device on your computer it has a different port number.
You can find out the number by running the following command:
```
make ports
```

### useful links
- no support megatinycores on pio yet: https://github.com/platformio/platform-atmelavr/issues/83#issuecomment-528268480

### programmer
- with simple ttl serial port: https://github.com/mraardvark/pyupdi
- https://github.com/SpenceKonde/megaTinyCore

### todos
- use cs on st7789 w/o CS: https://github.com/Bodmer/TFT_eSPI/issues/163#issuecomment-515616375
- decoupling caps: https://electronics.stackexchange.com/questions/90971/does-my-circuit-need-decoupling-caps
- use PTC (capacitive touch control) see http://ww1.microchip.com/downloads/en/appnotes/atmel-42360-ptc-robustness-design-guide_applicationnote_at09363.pdf

### toolchains for avr-gcc / avrxmega3
I tried many things including patching etc. At the end 2 options worked. You can configure them in the Makefile. I have markr42 active by default.

1. toolchain_microchip
- download the newest [toolchain](https://www.microchip.com/mplab/avr-support/avr-and-arm-toolchains-c-compilers) from microchip with a [patch](https://www.avrfreaks.net/comment/2838941#comment-2838941)
- download [pack](http://packs.download.atmel.com/) from atmel (search for attiny3217 to get the correct pack)
```
mkdir toolchain_microchip
tar xfz avr8-gnu-toolchain-osx-3.6.2.503-darwin.any.x86_64.tar.gz -C toolchai_microchip/ 
make patch_toolchain_microchip

mkdir toolchain_microchip/pack
unzip Atmel.ATtiny_DFP.1.4.283.atpack -d toolchain_microchip/pack/
```

2. toolchain_markr42 (more [info](https://www.avrfreaks.net/comment/2839521#comment-2839521)
I just adapted it to have the binaries locally installed and not in /usr/local/avr (that's what the patch is for)
```
mkdir toolchain_markr42
cd toolchain_markr42
brew install automake
wget https://github.com/MarkR42/robotbits/raw/master/avr_toolchain/build.sh
wget https://github.com/MarkR42/robotbits/raw/master/avr_toolchain/avr-xmega3.patch.bz
make patch_toolchain_markr42
```
