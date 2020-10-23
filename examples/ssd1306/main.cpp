#include <avr/io.h>
#include <string.h>
#include <stdlib.h>

#include "pins.h"
#include "uart.h"
#include "led.h"
#include "ssd1306.h"
#include "sleep.h"

/*
 * lcd power consumption in sleep mode: ~300uA
 * to have lower values, the screen must be shut down completely with a mosfet
 * this requires an init on every wakeup and the screen data is cleared though
 * for some strange reasons, the display consumes much more during sleep than in the datasheet
 */
int main(void) {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm); // set prescaler to 2 -> 10MHz
  // _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKOUT_bm); // output clk to PB5

  disable_buffer_of_pins(); // to save power

  DINIT();
  DF("\n\033[1;38;5;226;48;5;18m Hello from 0x%06lX \033[0m\n", get_deviceid());

  led_flash(&led_b, 3);

  ssd1306_init();
  ssd1306_dot(64, 0);
  ssd1306_char('a', 4, 0);
  ssd1306_text("Hello", 0, 0);
  ssd1306_text("World", 7, 0);
  ssd1306_hline(8, 0, 64, 4); // max width: 128
  sleep_s(2);
  ssd1306_off();
  sleep_s(2);
  ssd1306_on();
  ssd1306_clear(1, 0, 64); // clear horizontal line
  sleep_s(2);

  uint8_t humidity = 0; // value between 0-255
  uint8_t last_humidity = 0;
  int8_t increment = 10;
  while(1) {
    // DF("hum: %u, inc: %i\n", humidity, increment);
    last_humidity = humidity;
    if (((255-humidity) < increment && increment>=0) || (humidity < abs(increment) && increment<0)) {
      increment = -increment;
      humidity = increment > 0 ? 0 : 255;
    } else {
      humidity += increment;
    }

    if (last_humidity < humidity) {
      ssd1306_hline(8, last_humidity/2, (humidity-last_humidity)/2, 4); // max width: 128
    } else {
      ssd1306_clear(1, humidity/2, (humidity-last_humidity)/2);
    }

    sleep_ms(200);
  }
}
