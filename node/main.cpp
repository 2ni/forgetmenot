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

#define GATEWAY 99
#define NETWORK 33

TIMER timer;
TOUCH moist(&pin_moisture);

char feedback[RF69_MAX_DATA_LEN+1]; // 61 chars
char data[RF69_MAX_DATA_LEN+1];
uint8_t data_len;

uint8_t humidity, voltage;

void cmd_to_buffer(uint8_t *buffer, uint8_t pos, const char *cmd, uint16_t value) {
  uint8_t i = pos*4;
  buffer[i] = cmd[0];
  buffer[i+1] = cmd[1];
  buffer[i+2] = (value >> 8) & 0xff; // high byte
  buffer[i+3] = value & 0xff;
}

int main(void) {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm); // set prescaler to 2 -> 10MHz

  // deactivate all pins to save power
  clear_all_pins();

  // set some inputs
  set_direction(&multi, 0);               // to measure vcc of battery
  set_direction(&pin_temp_board, 0);
  set_direction(&pin_temp_moisture, 0);

  uint32_t deviceid = get_deviceid();
  DINIT();
  DF("Hello from 0x%06lX\n", deviceid);

  led_flash(&led_b, 3);

  uint8_t version = rfm69_init(868, deviceid, NETWORK);
  if (!version) {
    DL("RFM69 init failed\n");
  } else {
    DF("RFM69 Version: 0x%02x\n", version);
  }

  while(1) {
    char char_vcc[5], char_tb[5], char_tm[5];
    uint16_t vcc, tb, tm;
    vcc = get_vcc_battery();
    tb = get_temp_board();
    tm = get_temp_moist();
    uart_u2c(char_vcc, vcc, 2);
    uart_u2c(char_tb, tb, 1);
    uart_u2c(char_tm, tm, 1);

    uint16_t hum = moist.get_touch();
    uint16_t sleep_time = 5;

    data_len = sprintf(data, "tb:%s (%u) tm:%s (%u) h:%u vcc:%s (%u)", char_tb, tb, char_tm, tm, hum, char_vcc, vcc);
    DF("%s", data);

    led_flash(&led_b, 1);
    set_power_level(1);

    // we send junks of 4 bytes: 2 bytes cmd name, 2 bytes uint16_t value
    uint8_t num_of_cmds = 4;
    uint8_t buffer[4*num_of_cmds];
    cmd_to_buffer(buffer, 0, "tb", tb);
    cmd_to_buffer(buffer, 1, "tm", tm);
    cmd_to_buffer(buffer, 2, "hu", hum);
    cmd_to_buffer(buffer, 3, "vc", vcc);

    if (send_retry(GATEWAY, buffer, 4*num_of_cmds, 3, &timer)) {
      uint16_t rssi = get_data(feedback, RF69_MAX_DATA_LEN);
      DF(" feedback: '%s' [RSSI: %i]", feedback, rssi);
    } else {
      D(" feedback failed");
    }

    DL("");


    /*
    // send(GATEWAY, data, data_len, 0);
    if (send_retry(GATEWAY, data, data_len, 1, &timer)) {
      // send successful, get data from ack
      uint16_t rssi = get_data(feedback, RF69_MAX_DATA_LEN);

      char cmd;
      unsigned int cmd_value;
      sscanf(feedback, " %c:%u", &cmd, &cmd_value);
      DF("'%s' [RSSI: %i] [sleep: %u]\n", feedback, rssi, sleep_time);

    } else {
      DL("failed");
    }
    */

    rfm69_sleep();
    sleep_s(5);
  }
}
