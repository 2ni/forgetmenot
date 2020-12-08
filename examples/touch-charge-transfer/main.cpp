/*
 * implementation of charge transfer method
 * see https://bashtelorofscience.wordpress.com/2019/02/08/advanced-capacitive-sensing-with-arduino/
 * we use blk as gpio2 and cs_lcd as gpio1
 * blk    = PB0 (gpio2)
 * cs_lcd = PB2 (gpio1)
 *
 *      PB2
 *       |
 *    -------
 *    -------
 *       |
 *       |- PB0
 *       |
 *      ---
 *     touch
 *
 * charge transfer:
 * air:  1120
 * 25%:  200
 * 50%:  100
 * 75%:  65
 * 100%: 50
 *
 * charge compensation (adc)
 * air:  760
 * 25%:  800
 * 50%:  570
 * 75%:  460
 * 100%: 400
 *
 */

#include <util/delay.h>

#include "pins.h"
#include "uart.h"
#include "led.h"
#include "millis.h"
#include "sleep.h"

int main(void) {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm); // set prescaler to 2 -> 10MHz
  // _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKOUT_bm); // output clk to PB5

  DINIT();
  DF("\n\033[1;38;5;226;48;5;18m Hello from 0x%06lX\033[0m \n", get_deviceid());

  led_flash(&led_b, 3);

  /*
  #include "touch.h"

  TOUCH moist(&pin_moisture);
  uint16_t humidity=0;
  while (1) {
   humidity = moist.get_data();
   DT_I("humidity", humidity);
   _delay_ms(2000);
  }
  */

  uint16_t counter = 0;
  PORTB.DIRSET = PIN3_bm;

  while (1) {
    counter = 0;
    PORTB.OUTCLR = PIN3_bm;

    // discharge
    PORTB.DIRSET = PIN0_bm;
    PORTB.OUTCLR = PIN0_bm;
    PORTB.OUTCLR = PIN2_bm; // clear before setting to output to avoid short peak in voltage
    PORTB.DIRSET = PIN2_bm;
    _delay_us(10);

    while (1) {
        // fully charge both caps (volt difference says sth about the size)
      PORTB.DIRCLR = PIN0_bm;
      PORTB.DIRSET = PIN2_bm;
      PORTB.OUTSET = PIN2_bm;
      // _delay_us(1);
      while ((PORTB.IN & PIN0_bm) == 0);

      // shift load difference to gnd
      PORTB.DIRCLR = PIN2_bm;
      PORTB.DIRSET = PIN0_bm;
      PORTB.OUTCLR = PIN0_bm;
      //_delay_us(10);

      counter++;

      // measure if sum > threshold
      if (PORTB.IN & PIN2_bm) break;
    }

    // set pin high for oscilloscope trigger
    PORTB.OUTSET = PIN3_bm;
    DF("done. %u\n", counter);

    _delay_ms(2000);
  }
}
