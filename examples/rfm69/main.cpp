#include <util/delay.h>
#include <string.h>

#include "pins.h"
#include "uart.h"
#include "led.h"
#include "rfm69.h"
#include "spi.h"
#include "rfm69_registers.h"

#define NODE 2
#define TONODE 5
#define NETWORK 33


int main(void) {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm); // set prescaler to 2 -> 10MHz

  DINIT();
  DF("Hello from 0x%06lX", get_deviceid());

  led_setup();
  led_flash(&led_g, 3);


  // test spi by shorting MISO - MOSI
  /*
  spi_init();
  uint8_t test_byte = spi_transfer_byte(0x38);
  DF("test byte: 0x%02x", test_byte);
  */

  // test rfm69
  // () sth about pulldown on dio0 pin
  /*
  spi_init();
  set_output(&rfm_cs, 1);
  uint8_t reg = read_reg(REG_VERSION); // addr: 0x10 should be 0x24
  DF("Version: 0x%02x", reg);

  write_reg(REG_SYNCVALUE1, 0xaa);
  DF("SYNCVALUE1: 0x%02x", read_reg(REG_SYNCVALUE1));
  */

  uint8_t version = rfm69_init(868, NODE, NETWORK);
  if (!version) {
    DL("rfm69 init failed");
  } else {
    DF("RFM69 Version: 0x%02x", version);
  }

  // set_power_level(30);

  uint8_t counter = 0;
  char msg[8];


  while (1) {
    // node
    sprintf(msg, "Hello:%u", (counter++)%10);
    DF("%u->%u (%u): '%s'", NODE, TONODE, NETWORK, msg);
    send(TONODE, msg, strlen(msg), 0); // nodeid, buffer, size, ack
    _delay_ms(2000);


    // gateway
    /*
    if (receive_done()) {
      _delay_ms(10);
      if (ack_requested()) {
        DL("send ack");
        char ack[0];
        send_ack(ack, 0);
      }

      char data[16];
      get_data(data, 16);
      DF("received: %s", data);
    }
    */
  }
}
