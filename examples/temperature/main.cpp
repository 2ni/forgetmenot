#include <util/delay.h>

#include "uart.h"
#include "def.h"
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
  led_flash('g', 3);
  led_on('g');

  char buf[5];

  while (1) {
    led_toggle('b');
    led_toggle('g');


    uart_int2float(buf, measure_vcc(), 2);
    DF("vcc: %sv", buf);

    uart_int2float(buf, measure_temperature('b'), 1);
    DF("temp: %s\xc2\xb0", buf);

    DF("temp int: %u", measure_internal_temperature());

    _delay_ms(1000);
  }
}
