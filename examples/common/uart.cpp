/*
 * library to send debug information to UART (bit banging version)
 * only tx -> rx connection is needed
 *
 * pin DBG is used for this purpose
 *
 * use make serial to run UART
 *
 */

#include <stdlib.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include "uart.h"
#include "def.h"

/*
 * setup function which defines dbg pin
 */
void uart_setup(void) {
  // setup tx pin
  PORT_DBG.DIRSET = DBG;  // output
  TX_HIGH;

  _delay_ms(400); // ensure serial is ready
}

/*
 * send the variable name of a tuple to the uart
 */
void _uart_send_tuple_key(const char* key) {
  uart_send_string_p(key);
  uart_send_string_p(PSTR(": "));
}

/*
 * sends a tuple with type const char to the uart
 * ie "key: <const char>"
 */
void uart_tuple(const char* key, const char* value) {
  _uart_send_tuple_key(key);
  uart_send_string_p(value);
  uart_send_string_p(PSTR("\r\n"));
}

/*
 * sends a tuple with type uint16_t to the uart
 * ie "key: <uint16_t>"
 *
 * if hex (base=16) chosen, the output is shown as 0xAAAA
 */
void uart_tuple(const char* key, uint16_t value, uint8_t base) {
  _uart_send_tuple_key(key);
  char buf[6];
  if (base==16) {
    sprintf(buf, "%04x", value);
  } else {
    itoa(value, buf, base);
  }

  uart_send_string(buf);
  uart_send_string_p(PSTR("\r\n"));
}

/*
 * sends a tuple with type const char to the uart
 * ie "key: <char array>"
 */
void uart_tuple(const char* key, char* value) {
  _uart_send_tuple_key(key);
  uart_send_string(value);
  uart_send_string_p(PSTR("\r\n"));
}

/*
 * sends a single char to the uart
 */
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

/*
 * sends a char array to the uart
 * ie "key: <const char>"
 */
void uart_send_string(char* s) {
  while (*s) uart_send_char(*s++);
}

/*
 * sends a const char array to the uart
 */
void uart_send_string_p(const char* s) {
  while (pgm_read_byte(s)) uart_send_char(pgm_read_byte(s++));
}

/*
 * sends a uint16_t to the uart
 */
void uart_send_digit(uint16_t value, uint8_t base) {
  char buf[6];
  itoa(value, buf, base);
  uart_send_string(buf);
}
