#include <util/delay.h>
#include "pins.h"
#include "touch.h"
#include "uart.h"

TOUCH::TOUCH(pin_t *ipin) {
  pin = ipin;
  set_thresholds(get_avg());
}

TOUCH::TOUCH(pin_t *ipin, uint16_t threshold) {
  pin = ipin;
  set_thresholds(threshold);
}

void TOUCH::set_thresholds(uint16_t ithreshold) {
  threshold = ithreshold;
  threshold_upper = ithreshold + 30;
  threshold_lower = ithreshold + 15;
  DF("touch threshold: %u", threshold);
  DF("touch threshold upper: %u", threshold_upper);
  DF("touch threshold lower: %u", threshold_lower);
}

uint16_t TOUCH::get_avg() {
  uint16_t v, avg = 0;
  for(uint8_t i=0; i<10; i++) {
    v = get_touch();
    DF("v: %u", v);
    if (i>4) avg += v;
  }
  avg /= 5;

  return avg;
}

/*
 * get touch value of loaded Ct on Csh
 * returns a value between 0-1023
 * the higher the value the higher Ct
 * PC5 = AIN11
 */
uint16_t TOUCH::get_touch() {
  /*
  ADC1->CTRLC = ADC_PRESC_DIV64_gc | ADC_REFSEL_VDDREF_gc | (0<<ADC_SAMPCAP_bp);

  uint16_t result;
  PORTC.DIRSET = PIN5_bm; // set output and low to discharge touch
  ADC1.MUXPOS = ADC_MUXPOS_GND_gc; // discharge s/h cap by setting muxpos = gnd and run
  ADC1.CTRLA = (1<<ADC_ENABLE_bp) | (0<<ADC_FREERUN_bp) | ADC_RESSEL_10BIT_gc;
  ADC1.COMMAND |= 1;
  while (!(ADC1.INTFLAGS & ADC_RESRDY_bm));
  result = ADC1.RES;

  PORTC.DIRCLR = PIN5_bm; // set input with pullup to charge touch
  PORTC.PIN5CTRL |= PORT_PULLUPEN_bm;
  _delay_us(10);
  PORTC.PIN5CTRL &= ~PORT_PULLUPEN_bm; // disable pullup

  // enable adc, equalize s/h cap charge with touch charge
  ADC1.MUXPOS =  ADC_MUXPOS_AIN11_gc;
  ADC1.CTRLA = (1<<ADC_ENABLE_bp) | (0<<ADC_FREERUN_bp) | ADC_RESSEL_10BIT_gc;
  ADC1.COMMAND |= 1;
  while (!(ADC1.INTFLAGS & ADC_RESRDY_bm));
  result = ADC1.RES;

  ADC1.CTRLA = 0; // disable adc

  return result;
  */

  (*pin).port_adc->CTRLC = ADC_PRESC_DIV64_gc | ADC_REFSEL_VDDREF_gc | (0<<ADC_SAMPCAP_bp);

  uint16_t result;
  set_direction(pin, 0);           // set output and low to discharge touch
  result = get_adc(pin, ADC_MUXPOS_GND_gc); // discharge s/h cap by setting muxpos to gnd and run a measurement

  set_direction(pin, 1);           // set input with pullup to charge touch
  set_pullup(pin, 0);
  _delay_us(10);
  set_pullup(pin, 1);              // disable pullup

  // enable adc, equalize s/h cap charge with touch charge by setting muxpos and running measurement
  result = get_adc(pin);

  return result;
}

uint8_t TOUCH::is_pressed() {
  uint16_t v = get_touch();
  return v > threshold_upper;
}

uint8_t TOUCH::was_pressed() {
  uint16_t v = get_touch();

  if (v > (threshold_upper) && cleared) {
    cleared = 0;
    return 1;
  }

  if (v < (threshold_lower)) cleared = 1;
  return 0;
}
