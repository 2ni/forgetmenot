#include <util/delay.h>
#include <string.h>
#include <avr/interrupt.h>

#include "pins.h"
#include "uart.h"
#include "led.h"
#include "rfm95.h"
#include "sleep.h"
#include "sensors.h"

int main(void) {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm); // set prescaler to 2 -> 10MHz

  DINIT();
  uint32_t node = get_deviceid();
  DF("Hello from 0x%06lX\n", node);

  led_flash(&led_g, 3);
  led_on(&led_b);

  // test get_output
  DF("led green is: %u\n", get_output(&led_g));
  DF("led blue is: %u\n", get_output(&led_b));

  led_off(&led_b);

  uint8_t version = rfm95_init();
  if (!version) {
    DL("RFM95 init failed");
  } else {
    DF("RFM95 Version: 0x%02x\n", version);
  }

  uint16_t counter = 0x0000;
  uint16_t vcc, tb, tm;
  while(true) {
    uint8_t len = 6;
    uint8_t data[6];
    vcc = get_vcc_battery();
    tb = get_temp_board();
    tm = get_temp_moist();

    // msb first
    data[0] = (vcc >> 8) & 0xff;
    data[1] = vcc & 0xff;
    data[2] = (tb >> 8) & 0xff;
    data[3] = tb & 0xff;
    data[4] = (tm >> 8) & 0xff;
    data[5] = tm & 0xff;

    DL("sending");
    led_flash(&led_b, 1);
    lora_send_data(data, len, counter);
    counter++;

    sleep_s(60);
  }
}
