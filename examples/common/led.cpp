/*
 * getting started with attiny series 0/1
 * http://ww1.microchip.com/downloads/en/Appnotes/90003229A.pdf
 */

#ifndef __LED_H_
#define __LED_H__

#include <avr/io.h>

#include "led.h"

void led_setup() {
  led_g.port->DIRSET = (1<<led_g.pin);  // output
  led_g.port->OUTCLR = (1<<led_g.pin);  // set low

  led_b.port->DIRSET = (1<<led_b.pin);  // output
  led_b.port->OUTCLR = (1<<led_b.pin);  // set low

  /*
   * DBG used for now
   *
  PORT_LED.DIRSET = LED_R;  // output
  PORT_LED.OUTCLR = LED_R;  // set low
  */
}

void led_on(pin_t *led) {
  (*led).port->OUTSET = (1<<(*led).pin);
}

void led_on_all() {
  led_on(&led_g);
  led_on(&led_b);
}

uint8_t led_is_on(pin_t *led) {
  return (*led).port->IN & (1<<(*led).pin);
}

void led_off(pin_t *led) {
  (*led).port->OUTCLR = (1<<(*led).pin);
}

void led_off_all() {
  led_off(&led_g);
  led_off(&led_b);
}

void led_toggle(pin_t *led) {
  (*led).port->OUTTGL = (1<<(*led).pin);
}

void led_flash(pin_t *led, uint8_t num) {
  for (uint8_t c=0; c<num; c++) {
    led_on(led);
    _delay_ms(100);
    led_off(led);
    if (c!= num-1) _delay_ms(100); // no delay if last cycle
  }
}

#endif
