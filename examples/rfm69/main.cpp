#include <util/delay.h>
#include <string.h>
#include <avr/interrupt.h>

#include "pins.h"
#include "uart.h"
#include "led.h"
#include "rfm69.h"
#include "spi.h"
#include "rfm69_registers.h"

#define NODE 2
#define TONODE 5
#define NETWORK 33

volatile uint8_t timeout = 0;
ISR(TCA0_OVF_vect) {
  timeout = 1;
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm; // clear int flag
}

void timer_init() {
  TCA0.SINGLE.PER = 19532;                           // 10MHz/1024/19532 = 0.5Hz
  TCA0.SINGLE.INTCTRL |= TCA_SINGLE_OVF_bm;
}

void timer_start() {
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1024_gc |   // prescaler 1024
  TCA_SINGLE_ENABLE_bm;
}

/*
 * only clearing enable bit doesn't seem to work
 */
void timer_stop() {
  TCA0.SINGLE.CTRLA = 0;
}

int main(void) {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm); // set prescaler to 2 -> 10MHz

  DINIT();
  DF("Hello from 0x%06lX\n", get_deviceid());

  led_setup();
  led_flash(&led_g, 3);


  // test spi by shorting MISO - MOSI
  /*
  spi_init();
  uint8_t test_byte = spi_transfer_byte(0x38);
  DF("test byte: 0x%02x\n", test_byte);
  */

  // test rfm69
  // () sth about pulldown on dio0 pin
  /*
  spi_init();
  set_output(&rfm_cs, 1);
  uint8_t reg = read_reg(REG_VERSION); // addr: 0x10 should be 0x24
  DF("Version: 0x%02x\n", reg);

  write_reg(REG_SYNCVALUE1, 0xaa);
  DF("SYNCVALUE1: 0x%02x\n", read_reg(REG_SYNCVALUE1));
  */

  uint8_t version = rfm69_init(868, NODE, NETWORK);
  if (!version) {
    DL("rfm69 init failed");
  } else {
    DF("RFM69 Version: 0x%02x\n", version);
  }

  // set_power_level(30);

  uint8_t counter = 0;
  char msg[8];
  uint8_t ack_request = 1;

  timer_init();
  timer_start();
  sei();

  while (1) {
    // node
    if (timeout) {
      timer_stop();
      sprintf(msg, "Hello:%u", (counter++)%10);
      DF("%u->%u (%u): '%s'", NODE, TONODE, NETWORK, msg);
      send(TONODE, msg, strlen(msg), ack_request); // nodeid, buffer, size, ack
      if (ack_request) {
        while (!ack_received(TONODE)){}
        D(" ...got ack");
      }
      DLF();
      timeout = 0;
      timer_start();
    }

    /*
    if (receive_done() && ack_requested()) {
      D(" ...send ack");
      char ack[0];
      send_ack(ack, 0);
    }
    */

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
