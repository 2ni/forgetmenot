#include <util/delay.h>
#include "pins.h"
#include "touch.h"

#include "uart.h"

TOUCH::TOUCH(pin_t pin) {
  init(pin, get_avg());
}

TOUCH::TOUCH(pin_t pin, uint16_t threshold) {
  init(pin, threshold);
}

void TOUCH::init(pin_t ipin, uint16_t ithreshold) {
  threshold = ithreshold;
  pin = ipin;
  threshold_upper = ithreshold + 30;
  threshold_lower = ithreshold + 15;
  DF("touch threshold: %u", threshold);
}

uint16_t TOUCH::get_avg() {
  uint16_t avg = 0;
  for(uint8_t i=0; i<10; i++) {
    if (i>4) avg += get_touch();
  }
  avg /= 5;

  return avg;
}

/*
 * get touch value of loaded Ct on Csh
 * returns a value between 0-1023
 * the higher the value the higher Ct
 */
uint16_t TOUCH::get_touch() {
  pin.port_adc->CTRLC = ADC_PRESC_DIV64_gc | ADC_REFSEL_VDDREF_gc | (0<<ADC_SAMPCAP_bp);

  uint16_t result;
  set_direction(pin, 0);           // set output and low to discharge touch
  get_adc(pin, ADC_MUXPOS_GND_gc); // discharge s/h cap by setting muxpos to gnd and run a measurement

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
