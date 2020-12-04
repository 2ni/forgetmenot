#include "pins.h"
#include "uart.h"
#include "led.h"
#include "touch.h"
#include "millis.h"

int main(void) {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm); // set prescaler to 2 -> 10MHz
  // _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKOUT_bm); // output clk to PB5

  DINIT();
  DF("\n\033[1;38;5;226;48;5;18mHello from 0x%06lX\033[0m\n", get_deviceid());

  led_flash(&led_b, 3);

  millis_init(); // needed to detect long press

  TOUCH button(&pin_touch);
  uint8_t status;

  while (1) {
    // continuous mode
    /*
    if (button.is_pressed()) {
      DL("finger on touch");
    }
    */

    // toggle on any press (long or short)
    /*
    if (button.was_pressed()) {
      DL("pressed");
      led_toggle(&led_g);
    }
    */

    status = button.is_short_long();
    if (status == button.LONG) {
      DL("long press");
      led_toggle(&led_g);
    }
    if (status == button.SHORT) {
      DL("short press");
      led_toggle(&led_b);
    }
  }
}
