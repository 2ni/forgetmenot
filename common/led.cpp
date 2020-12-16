/*
 * led manipulation functions
 */

#ifndef __LED_H_
#define __LED_H__

#include <avr/io.h>

#include "led.h"
#include "sleep.h"
#include "pins.h"

void led_on(pin_t *led) {
  set_direction(led); // set output
  set_output(led, 1);
  // (*led).port->OUTSET = (1<<(*led).pin);
}

void led_on_all() {
  led_on(&led_g);
  led_on(&led_b);
}

uint8_t led_is_on(pin_t *led) {
  return get_output(led) == 1;
}

void led_off(pin_t *led) {
  set_direction(led);
  set_output(led, 0);
  // (*led).port->OUTCLR = (1<<(*led).pin);
}

void led_off_all() {
  led_off(&led_g);
  led_off(&led_b);
}

void led_toggle(pin_t *led) {
  set_direction(led);
  toggle_output(led);
  // (*led).port->OUTTGL = (1<<(*led).pin);
}

void led_flash(pin_t *led, uint8_t num) {
  for (uint8_t c=0; c<num; c++) {
    led_on(led);
    _s_sleep(20, 0); // avoid slow division with sleep_ms()
    led_off(led);
    if (c!= num-1) _s_sleep(102, 0); // no delay if last cycle
  }
}

#endif
