/*
 * getting started with attiny series 0/1
 * http://ww1.microchip.com/downloads/en/Appnotes/90003229A.pdf
 */

#ifndef __LED_H_
#define __LED_H__

#include <avr/io.h>
#include "led.h"

void led_setup() {
  PORT_LED.DIRSET = LED_G;  // output
  PORT_LED.OUTCLR = LED_G;  // set low

  PORT_LED.DIRSET = LED_B;  // output
  PORT_LED.OUTCLR = LED_B;  // set low

  /*
   * DBG used for now
   *
  PORT_LED.DIRSET = LED_R;  // output
  PORT_LED.OUTCLR = LED_R;  // set low
  */
}

void led_on(char color) {
  if (color == 'g') {
    PORT_LED.OUTSET = LED_G; // set high
  } else if (color == 'b') {
    PORT_LED.OUTSET = LED_B;
  } else if (color == 'r') {
    PORT_LED.OUTSET = LED_R;
  }
}

void led_on_all() {
  led_on('r');
  led_on('g');
  led_on('b');
}

int led_is_on(char color) {
  // if (~PORT_LED.IN & PIN2_bm)  // check if PA2 is low
  uint8_t val = 0;
  if (color == 'g') {
    val = PORT_LED.IN & LED_G;
  } else if (color == 'b') {
    val = PORT_LED.IN & LED_B;
  } else if (color == 'r') {
    val = PORT_LED.IN & LED_R;
  }

  return val;
}

void led_off(char color) {
  if (color == 'g') {
    PORT_LED.OUTCLR = LED_G; // set low
  } else if (color == 'b') {
    PORT_LED.OUTCLR = LED_B;
  } else if (color == 'r') {
    PORT_LED.OUTCLR = LED_R;
  }
}

void led_off_all() {
  // TODO faster: PORT_LED.OUT &= ~(1<<LED_G)...
  led_off('r');
  led_off('g');
  led_off('b');
}

void led_toggle(char color) {
  if (color == 'g') {
    PORT_LED.OUTTGL = LED_G; // invert (exclusive or)
  } else if (color == 'b') {
    PORT_LED.OUTTGL = LED_B;
  } else if (color == 'r') {
    PORT_LED.OUTTGL = LED_R;
  }
}

void led_flash(char color, uint8_t num) {
  for (uint8_t c=0; c<num; c++) {
    led_on(color);
    _delay_ms(100);
    led_off(color);
    if (c!= num-1) _delay_ms(100); // no delay if last cycle
  }
}

#endif
