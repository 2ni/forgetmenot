/*
 * sends sensor information to a rfm69 node or gateway
 * such as the python3-rfm69gateway
 * https://github.com/2ni/python3-rfm69gateway
 */

#include <string.h>
#include <util/delay.h>

#include "pins.h"
#include "uart.h"
#include "led.h"
#include "rfm69.h"
#include "timer.h"
#include "sleep.h"
#include "sensors.h"
#include "touch.h"

#define NODE 2
#define GATEWAY 99
#define NETWORK 33

TIMER timer;
TOUCH moist(&pin_moisture);

char feedback[RF69_MAX_DATA_LEN+1]; // 61 chars
char data[RF69_MAX_DATA_LEN+1];
uint8_t data_len;

uint8_t humidity, voltage;

int main(void) {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm); // set prescaler to 2 -> 10MHz

  // deactivate all pins to save power
  clear_all_pins();

  // set some inputs
  set_direction(&multi, 0);               // to measure vcc of battery
  set_direction(&pin_temp_board, 0);
  set_direction(&pin_temp_moisture, 0);

  DINIT();
  DF("Hello from 0x%06lX\n", get_deviceid());

  led_flash(&led_b, 3);

  uint8_t version = rfm69_init(868, NODE, NETWORK);
  if (!version) {
    DL("rfm69 init failed");
  } else {
    DF("RFM69 Version: 0x%02x\n", version);
  }

  while(1) {
    char vcc[5], tb[5], tm[5];
    uart_u2c(vcc, get_vcc_battery(), 2);
    uart_u2c(tb, get_temp_board(), 1);
    uart_u2c(tm, get_temp_moist(), 1);

    uint16_t hum = moist.get_touch();
    uint16_t sleep_time = 5;

    data_len = sprintf(data, "tb:%s|tm:%s|h:%u|vcc:%s", tb, tm, hum, vcc);
    // DF("%s\n", data);
    DF("%s response: ", data);
    led_flash(&led_b, 1);

    set_power_level(1);
    // send(GATEWAY, data, data_len, 0);
    if (send_retry(GATEWAY, data, data_len, 1, &timer)) {
      // send successful, get data from ack
      // feedback format: <cmd>:<value>|<cmd>:<value>|...
      // TODO extract commands from feedback
      uint16_t rssi = get_data(feedback, RF69_MAX_DATA_LEN);

      char cmd;
      unsigned int cmd_value;
      sscanf(feedback, " %c:%u", &cmd, &cmd_value);
      DF("'%s' [RSSI: %i] [sleep: %u]\n", feedback, rssi, sleep_time);

    } else {
      DL("failed");
    }

    rfm69_sleep();
    sleep_ms(1000);
  }
}
