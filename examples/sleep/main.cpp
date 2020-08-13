#include "pins.h"
#include "uart.h"
#include "led.h"
#include "sleep.h"

/*
 * simple standby example
 * periphery is off by default unless activated by RUNSTDBY
 * ports stay on in standby mode
 */
int main(void) {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm); // set prescaler to 2 -> 10MHz

  DINIT();
  DF("Hello from 0x%06lX\n", get_deviceid());

  led_setup();
  led_flash(&led_b, 3);

  while (1) {
    led_on(&led_b);
    sleep_ms(500);
    led_off(&led_b);

    sleep_ms(3000);
  }
}
