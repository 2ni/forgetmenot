#include <util/delay.h>
#include <string.h>

#include "pins.h"
#include "uart.h"
#include "led.h"
#include "timer.h"

TIMER timer;

int main(void) {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm); // set prescaler to 2 -> 10MHz
  // _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKOUT_bm); // output clk to PB5

  DINIT();
  DF("\n\033[1;38;5;226;48;5;18m Hello from 0x%06lX \033[0m\n", get_deviceid());

  led_flash(&led_b, 3);

  DL("starting timer. LED should light up in 5sec.");
  timer.start(5000);
  uint8_t done = 0;

  while (1) {
    if (timer.timed_out() && !done) {
      DL("timer finished");
      led_on(&led_g);
      timer.stop();
      done = 1;
    }
  }
}
