/*
 * getting started with attiny series 0/1
 * http://ww1.microchip.com/downloads/en/Appnotes/90003229A.pdf
 */

#ifndef __LED_H_
#define __LED_H__

#include <avr/io.h>
#include <util/delay.h>
#include "pins.h"

void led_setup();
void led_on(char color);
void led_on_all();
int led_is_on(char color);
void led_off(char color);
void led_off_all();
void led_toggle(char color);
void led_flash(char color, uint8_t num);

#endif
