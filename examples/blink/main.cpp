#include <avr/io.h>
// #include <avr/interrupt.h>
#include <util/delay.h>
#include "uart.h"
#include "def.h"
#include <string.h>

int main(void) {
  DINIT(); // simplex uart setup
  DL("Hello there");

  led_setup();

  while (1) {
    // debug output examples
    uint16_t time = 14530;        // 0x38C2
    DT_I("uint", time);           // uint: 14530
    DT_IH("uint hex", time);      // uint hex: 0x38C2
    DF(10, "time:%u\n", time);    // time: 14530
    DT_C("fixed char", "value");  // char: value
    char var[5];
    strcpy(var, "test");
    DT_S("variable char", var);   // variable char: test

    // blink the led
    led_toggle('r');
    _delay_ms(500);
  }
}
