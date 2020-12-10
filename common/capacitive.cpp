/*
 *
 *      pin2
 *       |
 *    -------
 *    ------- Ccharge
 *       |
 *       |- pin0
 *       |
 *      ---
 *     Ctouch
 */

#include <util/delay.h>

#include "capacitive.h"
#include "pins.h"
#include "uart.h"
#include "sensors.h"

CAPACITIVE::CAPACITIVE(pin_t *ipin0, pin_t *ipin2) {
  pin0 = ipin0;
  pin2 = ipin2;
}

uint16_t CAPACITIVE::get_humidity(uint8_t relative) {
  uint16_t cycles = 0; // cycles needed to charge Ccharge from Ctouch to threshold of pin2

  // discharge
  set_output(pin0, 0);
  set_direction(pin0, 1);
  set_output(pin2, 0);
  set_direction(pin2, 1);
  _delay_us(10);

  while (1) {
      // fully charge both caps (volt difference says sth about the size)
    set_direction(pin0, 0);
    set_direction(pin2, 1);
    set_output(pin2, 1);
    while (!get_output(pin0));

    // shift load difference to gnd
    set_direction(pin2, 0);
    set_direction(pin0, 1);
    set_output(pin0, 0);

    cycles++;

    // measure if sum > threshold
    if (cycles == 1000 || get_output(pin2)) break;
  }

  return relative ? to_relative(cycles) : cycles;
}

/*
 * Ccharge = 200nF:
 * air:  2000
 * 12%:  500
 * 25%:  350
 * 37%:  200
 * 50%:  160
 * 63%.  120
 * 75%:  110
 * 88%:  95
 * 100%: 85
 */
uint8_t CAPACITIVE::to_relative(uint16_t value) {
  const temp_characteristics_struct humidity_characteristics[] = {
    {0, 1000},
    {12, 500},
    {25, 350},
    {37, 200},
    {50, 160},
    {63, 120},
    {75, 110},
    {88, 95},
    {100, 85}
  };

  return (uint8_t)interpolate(value, humidity_characteristics, 9);
}
