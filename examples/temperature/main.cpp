#include <util/delay.h>

#include "pins.h"
#include "uart.h"
#include "led.h"
#include "sensors.h"

/*
 * output voltage on chip and temperature from sensor and chip
 */

int main(void) {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm); // set prescaler to 2 -> 10MHz

  DINIT();
  DL("Hello.");

  led_setup();
  led_flash(&led_g, 3);
  led_on(&led_g);

  char buf[5];

  while (1) {
    led_toggle(&led_g);
    led_toggle(&led_b);


    uart_int2float(buf, get_vcc(), 2);
    DF("vcc: %sv", buf);

    uart_int2float(buf, get_temp_board(), 1);
    DF("temp board: %s\xc2\xb0", buf);

    uart_int2float(buf, get_temp_moist(), 1);
    DF("temp moist: %s\xc2\xb0", buf);

    DF("temp chip: %u", get_temp_chip());

    _delay_ms(1000);
  }
}
