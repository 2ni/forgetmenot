#include <avr/io.h>
// #include <avr/interrupt.h>
#include <util/delay.h>
#include "uart.h"
#include "def.h"

int main(void) {
  DINIT(); // simplex uart setup
  DL("Hello there");

  led_setup();

  while (1) {
    _delay_ms(500);
    uint8_t time = 12;
    DF("time:%u\n", time);
    DL("blink");
    led_toggle('r');
  }
}
