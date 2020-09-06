#include <util/delay.h>
#include <string.h>
#include <avr/interrupt.h>

#include "pins.h"
#include "uart.h"
#include "led.h"
#include "sleep.h"

int main(void) {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm); // set prescaler to 2 -> 10MHz
  // _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKOUT_bm); // output clk to PB5

  DINIT();
  DF("Hello from 0x%06lX\n", get_deviceid());

  led_flash(&led_b, 3);

  while(1) {
    led_toggle(&led_g);
    DL("ping");
    sleep_s(3);
  }
}
