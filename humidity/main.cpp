/*
 * LoRaWAN node to measure humidity
 * capacitive humidity sensor with Ccharge ~200nF (see capacitive.cpp)
 * show it on a SSD1306 attached to i2c (needed or program will be stuck)
 * send to ttn is not yet possible as sda, scl shared with miso, mosi
 */

#include <avr/eeprom.h>

#include "pins.h"
#include "uart.h"
#include "led.h"
#include "sensors.h"
#include "sleep.h"
#include "touch.h"
#include "capacitive.h"
#include "millis.h"
#include "ssd1306.h"
#include "sensors.h"
#include "lorawan.h"

enum MODES {
  SLEEP = 0,
  ACTIVITY = 1,
  CALIBRATE = 2,
  MEASURE = 3
};


#define TIMEOUT_LONG 3000      // ms needed pressing the touch button to get into calibration mode
#define TIMEOUT_DISPLAY 10000  // ms while the display is on if no interaction
#define MEASURE_INTERVALL 1000 // ms until data is updated while displayed
#define SLEEP_TIME 100         // ms while mcu is sleeping in loops
#define _MEASURE_NUMBER MEASURE_INTERVALL/SLEEP_TIME
#define _DISPLAY_NUMBER TIMEOUT_DISPLAY/MEASURE_INTERVALL

#define DATA_COL 9*6 // position of data (char width = 6 points)
#define DATA_LEN 5*6 // 4 chars

uint8_t EEMEM ee_humidity_target;
Lora_session EEMEM ee_session;

void oled_screen_data() {
  ssd1306_clear();
  ssd1306_text("Moisture sensor", 0, 0);
  ssd1306_text("Humidity", 2, 0);
  ssd1306_text("Tsensor", 3, 0);
  ssd1306_text("Tboard", 4, 0);
  ssd1306_text("Vcc", 5, 0);
}

void oled_screen_calibrating() {
  ssd1306_clear();
  ssd1306_text("CALIBRATING", 2, 31);
}

int main(void) {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm); // set prescaler to 2 -> 10MHz
  // _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKOUT_bm); // output clk to PB5

  // save power except for PC5 (touch), PB0 & PB2 (humidity sensor)
  // TODO tonr on/off buffer when wakeup/sleep
  disable_buffer_of_pins();
  PORTC.PIN5CTRL = 0;
  PORTB.PIN0CTRL = 0;
  PORTB.PIN2CTRL = 0;

  DINIT();
  uint32_t deviceid = get_deviceid();
  DF("\n\033[1;38;5;226;48;5;18mHello from 0x%06lX\033[0m\n", deviceid);

  led_flash(&led_b, 3);

  millis_init();
  ssd1306_init();

  uint8_t mode = SLEEP;
  uint8_t last_mode = SLEEP;
  uint8_t new_mode = MEASURE;

  uint8_t measure_counter = 0;
  uint8_t display_counter = 0;

  uint8_t h_rel_target = 0;
  uint8_t h_rel_target_last = 0;
  // uint32_t now = millis_time();
  uint8_t button_status;

  char temp_board[6] = {0};
  char temp_sensor[6] = {0};
  char humidity[5] = {0};
  char vcc[5];

  uint8_t curpos = 0;

  uint8_t h_rel = 0;
  uint8_t h_rel_last = 0;

  h_rel_target = eeprom_read_byte(&ee_humidity_target);
  if (h_rel_target == 255) h_rel_target = 50;
  h_rel_target_last = h_rel_target;

  TOUCH button(&pin_touch);
  CAPACITIVE moisture(&lcd_blk, &lcd_cs);

  /*
  while (1) {
    if ((millis_time()-now) > 3000) {
      uart_u2c(humidity, moisture.get_humidity(1), 0);
      uart_u2c(temp_sensor, get_temp_moist(), 1);
      uart_u2c(temp_board, get_temp_board(), 1);
      uart_u2c(vcc, get_vcc_battery(), 2);

      DF("humidity: %s%%, target: %u%%\n", humidity, h_rel_target);
      DF("Tsensor : %s\n", temp_sensor);
      DF("Tboard: %s\n", temp_board);
      DF("Vcc     : %s\n", vcc);
      // DF("moist: %s%%, temp:%s\n", humidity, temp_board);
      now = millis_time();
    }
  }
  */

  while (1) {
    /*
    if (mode != last_mode) {
      DF("\nmode: %u\n", mode);
    }
    */

    last_mode = mode;
    mode = new_mode;
    switch(mode) {
      case SLEEP:
        if (button.is_pressed()) {
          led_flash(&led_g, 1);
          ssd1306_on();
          new_mode = button.pressed(TIMEOUT_LONG) == button.LONG ? CALIBRATE : MEASURE;
        } else {
          sleep_ms(SLEEP_TIME);
        }
        break;

      case MEASURE:
        if (last_mode != mode) {
          oled_screen_data();
          ssd1306_clear(6, 14+h_rel_target_last, 1);
          ssd1306_dot(14+h_rel_target, 50, 4);
          h_rel_target_last = h_rel_target;
          h_rel = 0;
          measure_counter = 0;
          display_counter = 0;
        }

        if (measure_counter == 0) {
          h_rel_last = h_rel;
          h_rel = moisture.get_humidity(1);
          uart_u2c(humidity, h_rel, 0);
          ssd1306_clear(2, DATA_COL, 2*DATA_LEN+1);
          curpos = ssd1306_text(humidity, 2, DATA_COL);
          curpos = ssd1306_text("%", 2, curpos);
          uart_u2c(humidity, h_rel_target, 0);
          curpos = ssd1306_text("(", 2, curpos+2);
          curpos = ssd1306_text(humidity, 2, curpos+2);
          ssd1306_text("%)", 2, curpos);

          // moisture shown as bar from 14px - 114px
          if (h_rel > h_rel_last) {
            ssd1306_hline(56, 14+h_rel_last, (h_rel-h_rel_last), 8);
          } else {
            ssd1306_clear(7, 14+h_rel, (h_rel_last-h_rel));
          }

          uart_u2c(temp_sensor, get_temp_moist(), 1);
          ssd1306_clear(3, DATA_COL, DATA_LEN);
          curpos = ssd1306_text(temp_sensor, 3, DATA_COL);
          ssd1306_text("~", 3, curpos);

          uart_u2c(temp_board, get_temp_board(), 1);
          ssd1306_clear(4, DATA_COL, DATA_LEN);
          curpos = ssd1306_text(temp_board, 4, DATA_COL);
          ssd1306_text("~", 4, curpos);

          uart_u2c(vcc, get_vcc_battery(), 2);
          ssd1306_clear(5, DATA_COL, DATA_LEN);
          curpos = ssd1306_text(vcc, 5, DATA_COL);
          ssd1306_text("v", 5, curpos);

          DF("humidity: %u%%, target: %u%%\n", h_rel, h_rel_target);
          DF("Tsensor : %s\n", temp_sensor);
          DF("Tboard  : %s\n", temp_board);
          DF("Vcc     : %s\n", vcc);
        }
        sleep_ms(SLEEP_TIME);

        button_status = button.pressed(TIMEOUT_LONG); // TODO avoid blocking
        if (button_status == button.SHORT) {
          led_flash(&led_g, 1);
          ssd1306_off();
          new_mode = SLEEP;
          /*
          // if click should not put it to sleep but extend measurement
          measure_counter = 0;
          display_counter = 0;
          */
        } else if (button_status == button.LONG) {
          new_mode = CALIBRATE;
        }

        if (++measure_counter == _MEASURE_NUMBER) {
          measure_counter = 0;
          if (++display_counter == _DISPLAY_NUMBER) {
            ssd1306_off();
            new_mode = SLEEP;
          }
        }
        break;

      case CALIBRATE:
        if (last_mode != mode) {
          led_on(&led_b);
          oled_screen_calibrating();
          measure_counter = 0;
          display_counter = 0;
        }

        if (measure_counter == 0) {
          h_rel_target = moisture.get_humidity(1);
          uart_u2c(humidity, h_rel_target, 0);
          ssd1306_clear(3, 64-9, 3*6);
          curpos = ssd1306_text(humidity, 3, 64-9);
          ssd1306_text("%", 3, curpos);
        }
        sleep_ms(SLEEP_TIME);

        if (!button.is_pressed()) {
          eeprom_write_byte(&ee_humidity_target, h_rel_target);
          h_rel = 0;
          led_off(&led_b);
          new_mode = MEASURE;
        }

        if (++measure_counter == _MEASURE_NUMBER) {
          measure_counter = 0;
        }
        break;
    }
  }
}
