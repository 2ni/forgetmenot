/*
 * getting started with attiny series 0/1
 * http://ww1.microchip.com/downloads/en/Appnotes/90003229A.pdf
 */

#ifndef __DEF_H__
#define __DEF_H__

#include <avr/io.h>

#define PA1 1
#define PA2 2
#define PA3 3
#define PA4 4
#define PA5 5
#define PA6 6
#define PA7 7
#define PA8 8
#define LED_G PA1
#define LED_B PA2
#define LED_R PA3

inline static void led_setup() {
  PORTA.DIRSET |= (1<<LED_G);  // output
  PORTA.OUTCLR |= (1<<LED_G);  // set low

  PORTA.DIRSET |= (1<<LED_B);  // output
  PORTA.OUTCLR |= (1<<LED_B);  // set low

  PORTA.DIRSET |= (1<<LED_R);  // output
  PORTA.OUTCLR |= (1<<LED_R);  // set low
}

inline static void led_on(char color) {
  if (color == 'g') {
    PORTA.OUT |= (1<<LED_G); // set high
  } else if (color == 'b') {
    PORTA.OUT |= (1<<LED_B);
  } else if (color == 'r') {
    PORTA.OUT |= (1<<LED_R);
  }
}

inline static void led_on_all() {
  led_on('r');
  led_on('g');
  led_on('b');
}

inline static int led_is_on(char color) {
  // if (~PORTA.IN & PIN2_bm)  // check if PA2 is low
  uint8_t val;
  if (color == 'g') {
    val = PORTA.OUT & (1<<LED_G);
  } else if (color == 'b') {
    val = PORTA.OUT & (1<<LED_B);
  } else if (color == 'r') {
    val = PORTA.OUT & (1<<LED_R);
  }

  return val;
}

inline static void led_off(char color) {
  if (color == 'g') {
    PORTA.OUT &= ~(1<<LED_G); // set low
  } else if (color == 'b') {
    PORTA.OUT &= ~(1<<LED_B);
  } else if (color == 'r') {
    PORTA.OUT &= ~(1<<LED_R);
  }
}

inline static void led_off_all() {
  // TODO faster: PORTA.OUT &= ~(1<<LED_G)...
  led_off('r');
  led_off('g');
  led_off('b');
}

inline static void led_toggle(char color) {
  if (color == 'g') {
    PORTA.OUT ^= (1<<LED_G); // invert (exclusive or)
  } else if (color == 'b') {
    PORTA.OUT ^= (1<<LED_B);
  } else if (color == 'r') {
    PORTA.OUT ^= (1<<LED_R);
  }
}

inline static void led_flash(char color, uint8_t num) {
  for (uint8_t c=0; c<num; c++) {
    led_on(color);
    _delay_ms(10);
    led_off(color);
    if (c!= num-1) _delay_ms(200); // no delay if last cycle
  }
}

#endif
