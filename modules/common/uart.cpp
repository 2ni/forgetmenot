/*
 * DBG_PORT, DBG_DDR, DBG must be defined
 * to a pin on which TXD is connected to
 */

#include <stdlib.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include "uart.h"
#include "def.h"

uint8_t uart_setup(void) {
  // setup tx pin
  PORT_DBG.DIRSET = DBG;  // output
  TX_HIGH;

  _delay_ms(400); // ensure serial is ready
  return 0;
}

void uart_send_char(char c) {
  uint8_t i = 8;
  // start bit
  TX_LOW; UART_DELAY;
  while(i){
    if(c & 1){
      TX_HIGH; UART_DELAY;
    } else {
      TX_LOW; UART_DELAY;
    }
    c = c >> 1;
    i--;
  }
  // stop bits
  TX_HIGH; UART_DELAY;
}

void uart_send_string(char* s) {
  while (*s) uart_send_char(*s++);
}

void uart_send_string_p(const char* s) {
  while (pgm_read_byte(s)) uart_send_char(pgm_read_byte(s++));
}

void uart_send_digit(uint16_t value) {
  char buf[6];
  itoa(value, buf, 10);
  uart_send_string(buf);
}
