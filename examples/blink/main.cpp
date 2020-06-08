#include <util/delay.h>
#include <string.h>

#include "uart.h"
#include "def.h"

int main(void) {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm); // set prescaler to 2 -> 10MHz
  // _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKOUT_bm); // output clk to PB5

  DINIT();

  led_setup();
  led_flash('g', 3);
  led_on('g');

  // debug output examples
  uint16_t time = 14530;        // 0x38C2
  DT_I("uint", time);           // uint: 14530
  DT_IH("uint hex", time);      // uint hex: 0x38C2
  DF("time:%u", time);    // time: 14530
  DT_C("fixed char", "value");  // char: value
  char var[5];
  strcpy(var, "test");
  DT_S("variable char", var);   // variable char: test

  while (1) {
    led_toggle('b');
    led_toggle('g');

    _delay_ms(500);
  }
}
