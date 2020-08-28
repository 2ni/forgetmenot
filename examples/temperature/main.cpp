#include <util/delay.h>

#include "pins.h"
#include "uart.h"
#include "led.h"
#include "sensors.h"
#include "sleep.h"

/*
 * output voltages from cpu and battery
 * temperatures from board, sensor and cpu
 */

int main(void) {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm); // set prescaler to 2 -> 10MHz

  clear_all_pins();

  // set needed pins as input
  set_direction(&multi, 0);
  set_direction(&pin_temp_board, 0);
  set_direction(&pin_temp_moisture, 0);

  DINIT();
  DF("Hello from 0x%06lX\n", get_deviceid());

  led_flash(&led_g, 3);
  led_on(&led_g);

  char vcc_bat[5], vcc_cpu[5];
  uint16_t vcc_cpu_raw, vcc_bat_raw;

  char temp_board[5], temp_moist[5];
  uint16_t temp_cpu, temp_board_raw, temp_moist_raw;

  while (1) {
    led_toggle(&led_g);
    led_toggle(&led_b);

    // get voltages
    vcc_cpu_raw = get_vcc_cpu();
    uart_int2float(vcc_cpu, vcc_cpu_raw, 2);

    vcc_bat_raw = get_vcc_battery();
    uart_int2float(vcc_bat, vcc_bat_raw, 2);

    DF("vcc cpu: %sv (%u), battery: %sv (%u)\n", vcc_cpu, vcc_cpu_raw, vcc_bat, vcc_bat_raw);

    // get temperatures
    temp_cpu = get_temp_cpu();

    temp_board_raw = get_temp_board();
    uart_int2float(temp_board, temp_board_raw, 1);

    temp_moist_raw = get_temp_moist();
    uart_int2float(temp_moist, temp_moist_raw, 1);

    DF("temp cpu: %u, board: %s (%u), moisture: %s (%u)\n", temp_cpu, temp_board, temp_board_raw, temp_moist, temp_moist_raw);

    sleep_ms(2000);
  }
}
