/*
 * Simple capacitive touch based on adc
 * inspired by
 * - https://github.com/nerdralph/nerdralph/blob/master/avr/adctouch.c
 * - https://github.com/tuomasnylund/avr-capacitive-touch/blob/master/code/touch.c
 * - https://github.com/martin2250/ADCTouch/blob/master/src/ADCTouch.cpp
 *
 * Ctouch: capacity on touch pin
 * Csh   : capacity s/h of adc (sample and hold)
 *
 *        Ctouch
 * Vsh = --------   * Vcc
 *      (Ctouch+Csh)
 *
 * measurement:
 * 1. discharge Csh and Ctouch
 * 2. charge Ctouch to Vcc
 * 3. disable charge (pullup)
 * 4. connect Ctouch to Csh (by enabling adc
 * 5. measure voltage. the higher the value, the higher Ctouch
 *
 */

#include <util/delay.h>
#include <string.h>

#include "pins.h"
#include "uart.h"
#include "led.h"
#include "touch.h"

/*
 * this function was integrated into TOUCH but is kept here for testing purposes
 */
uint16_t touch() {
  pin_touch.port_adc->CTRLC = ADC_PRESC_DIV64_gc | ADC_REFSEL_VDDREF_gc | (0<<ADC_SAMPCAP_bp);

  uint16_t result;
  set_direction(pin_touch, 0);           // set output and low to discharge touch
  get_adc(pin_touch, ADC_MUXPOS_GND_gc); // discharge s/h cap by setting muxpos to gnd and run a measurement

  set_direction(pin_touch, 1);           // set input with pullup to charge touch
  set_pullup(pin_touch, 0);
  _delay_us(10);
  set_pullup(pin_touch, 1);              // disable pullup

  // enable adc, equalize s/h cap charge with touch charge by setting muxpos and running measurement
  result = get_adc(pin_touch);

  return result;
}

int main(void) {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm); // set prescaler to 2 -> 10MHz

  DINIT();
  DL("Hello.");

  led_setup();
  led_flash('g', 3);

  // use this class or the commented function below
  TOUCH button(pin_touch);

  while(1) {
    if (button.was_pressed()) {
      led_on('g');
      _delay_ms(1000);
      led_off('g');
    }
  }

  /*
  touch_init();

  uint16_t avg = 0;
  for(uint8_t i=0; i<10; i++) {
    if (i>4) avg += touch();
  }
  avg /= 5;
  DF("avg: %u", avg);

  uint16_t threshold_upper = avg + 30;
  uint16_t threshold_lower = avg + 15;
  uint16_t v;
  uint8_t touch_clr = 1;

  while(1) {
    v = touch();
    if (v>threshold_upper && touch_clr) {
      touch_clr = 0;
      led_on('g');
      _delay_ms(1000);
      led_off('g');
    }

    if (v<threshold_lower) touch_clr = 1;

    // DF("value: %u", v);
    // _delay_ms(200);
  }
  */
}
