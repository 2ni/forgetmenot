/*
 * getting started with attiny series 0/1
 * http://ww1.microchip.com/downloads/en/Appnotes/90003229A.pdf
 */

#ifndef __DEF_H__
#define __DEF_H__

#include <avr/io.h>

// PORT A
#define MOSI       PIN1_bm
#define MISO       PIN2_bm
#define SCK        PIN3_bm
#define CS_RFM     PIN4_bm
#define DIO0       PIN5_bm
#define DIO1       PIN6_bm
#define OUT2       PIN7_bm

#define PORT_SPI   PORTA
#define PORT_RFM   PORTA

// PORT B
#define BLK        PIN0_bm
#define DBG        PIN1_bm
#define LED_R      PIN1_bm
#define CS_LCD     PIN2_bm
#define DC         PIN3_bm
#define RST        PIN4_bm
#define LED_B      PIN5_bm
#define LED_G      PIN6_bm
#define OUT1       PIN7_bm

#define PORT_LED   PORTB
#define PORT_DBG   PORTB
#define PORT_LCD   PORTB

// PORT C
#define TEMP1      PIN0_bm
#define MOIST      PIN1_bm
#define TEMP2      PIN2_bm
#define HALL       PIN3_bm
#define MULTI      PIN4_bm
#define TOUCH      PIN5_bm

#define PORT_MOIST PORTC
#define PORT_TEMP  PORTC
#define PORT_HALL  PORTC
#define PORT_TOUCH PORTC
#define PORT_HALL  PORTC

inline static void led_setup() {
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

inline static void led_on(char color) {
  if (color == 'g') {
    PORT_LED.OUTSET = LED_G; // set high
  } else if (color == 'b') {
    PORT_LED.OUTSET = LED_B;
  } else if (color == 'r') {
    PORT_LED.OUTSET = LED_R;
  }
}

inline static void led_on_all() {
  led_on('r');
  led_on('g');
  led_on('b');
}

inline static int led_is_on(char color) {
  // if (~PORT_LED.IN & PIN2_bm)  // check if PA2 is low
  uint8_t val;
  if (color == 'g') {
    val = PORT_LED.IN & LED_G;
  } else if (color == 'b') {
    val = PORT_LED.IN & LED_B;
  } else if (color == 'r') {
    val = PORT_LED.IN & LED_R;
  }

  return val;
}

inline static void led_off(char color) {
  if (color == 'g') {
    PORT_LED.OUTCLR = LED_G; // set low
  } else if (color == 'b') {
    PORT_LED.OUTCLR = LED_B;
  } else if (color == 'r') {
    PORT_LED.OUTCLR = LED_R;
  }
}

inline static void led_off_all() {
  // TODO faster: PORT_LED.OUT &= ~(1<<LED_G)...
  led_off('r');
  led_off('g');
  led_off('b');
}

inline static void led_toggle(char color) {
  if (color == 'g') {
    PORT_LED.OUTTGL = LED_G; // invert (exclusive or)
  } else if (color == 'b') {
    PORT_LED.OUTTGL = LED_B;
  } else if (color == 'r') {
    PORT_LED.OUTTGL = LED_R;
  }
}

inline static void led_flash(char color, uint8_t num) {
  for (uint8_t c=0; c<num; c++) {
    led_on(color);
    _delay_ms(100);
    led_off(color);
    if (c!= num-1) _delay_ms(100); // no delay if last cycle
  }
}

#endif
