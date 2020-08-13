#include <util/delay.h>
#include <string.h>
#include <avr/interrupt.h>

#include "pins.h"
#include "uart.h"
#include "led.h"
#include "rfm69.h"
#include "spi.h"
#include "rfm69_registers.h"
#include "timer.h"

#define NODE 2
#define TONODE 5
#define NETWORK 33

/*
volatile uint8_t timeout = 0;
ISR(TCA0_OVF_vect) {
  timeout = 1;
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm; // clear int flag
}

void timer_init() {
  TCA0.SINGLE.INTCTRL |= TCA_SINGLE_OVF_bm;
}

void timer_start(uint16_t ms) {
  timeout = 0;
  uint32_t ticks = ((10e6*ms)/1024)/1000;
  TCA0.SINGLE.PER = ticks;                           // 10MHz/1024/19532 = 0.5Hz
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1024_gc |   // prescaler 1024
  TCA_SINGLE_ENABLE_bm;
}

void timer_stop() {
  TCA0.SINGLE.CTRLA = 0; // only clearing enable bit doesn't seem to work
}

volatile uint8_t timeoutb = 0;
void timerb_init() {
  TCB0.CCMP = 9776; // 10MHz/1024/4883 = 1Hz
  TCB0.INTCTRL = TCB_CAPT_bm;
}

void timerb_start() {
  timeoutb = 0;
  TCB0.CTRLA = TCB_ENABLE_bm | TCB_CLKSEL_CLKTCA_gc;  // use clk from tca 10MHz/1024
}

void timerb_stop() {
  TCB0.CTRLA = 0;
}

ISR(TCB0_INT_vect) {
  timeoutb = 1;
  TCB0.INTFLAGS = TCB_CAPT_bm;
}
*/

TIMER timer;


int main(void) {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm); // set prescaler to 2 -> 10MHz

  DINIT();
  DF("Hello from 0x%06lX\n", get_deviceid());

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
  char msg[9];
  // uint8_t ack_request = 1;
  char incoming[RF69_MAX_DATA_LEN+1];
  int16_t rssi;
  uint16_t sleep_time = 2000;

  char cmd;
  unsigned int value;

  timer.start(2000);
  while (1) {
    // node
    if (timer.timed_out()) {
      timer.stop();
      uint8_t len = sprintf(msg, "Hello:%u", (counter++)%10);

      DF("send_retry: %u->%u (%u): '%s'", NODE, TONODE, NETWORK, msg);
      if (send_retry(TONODE, msg, len, 3, &timer)) {
        rssi = get_data(incoming, RF69_MAX_DATA_LEN);
        sscanf(incoming, " %c:%u", &cmd, &value);
        if (cmd=='s') {
          sleep_time = value * 1000; // convert in ms
          // DF("((%c, %u))", cmd, value);
        }
        /*
        char * token;
        token = strtok(incoming, "|");
        while (token) {
        }
        */
        DF(" ...ok with response: '%s' [RSSI: %i] [sleep: %u]\n", incoming, rssi, sleep_time);
      } else {
        DL(" ...failed");
      }

      /*
      DF("send: %u->%u (%u): '%s'", NODE, TONODE, NETWORK, msg);
      send(TONODE, msg, strlen(msg), ack_request); // nodeid, buffer, size, ack
      if (ack_request) {
        timer.start(1000);
        while (!timer.timed_out() && !ack_received(TONODE)) {}
        if (timer.timed_out()) {
          D(" ...timeout");
        } else {
          get_data(data, RF69_MAX_DATA_LEN);
          DF(" ...got ack with response: '%s'", data);
        }
      }
      DLF();
      */

      sleep();
      timer.start(sleep_time);
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
