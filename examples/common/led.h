/*
 * getting started with attiny series 0/1
 * http://ww1.microchip.com/downloads/en/Appnotes/90003229A.pdf
 *
 * note that eg sleep function clear any pin status (clear_all_pins())
 * therefore the pin status is set in any function
 */

#ifndef __LED_H_
#define __LED_H__

#include <avr/io.h>
#include <util/delay.h>
#include "pins.h"

void led_on(pin_t *led);
void led_on_all();
uint8_t led_is_on(pin_t *led);
void led_off(pin_t *led);
void led_off_all();
void led_toggle(pin_t *led);
void led_flash(pin_t *led, uint8_t num);

#endif
